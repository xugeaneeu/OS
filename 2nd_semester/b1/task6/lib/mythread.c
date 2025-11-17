#define _GNU_SOURCE

#include "mythread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <stdatomic.h>
#include <sys/param.h>
#include <pthread.h>


/* --- configuration --- */
#define STACK_SIZE (1<<20) /* 1 MiB stack per thread */

/* --- internal structures --- */

typedef struct cleanup_node {
    void (*rtn)(void *);
    void *arg;
    struct cleanup_node *next;
} cleanup_node;

typedef struct thread_info {
    unsigned long tid;
    void *stack_base;             /* base returned by mmap (includes guard page) */
    size_t stack_total_size;      /* mmap size = guard + stack */
    atomic_int finished;          /* 0/1 */
    void *retval;
    atomic_int futex;             /* 0 initially, set to 1 on finish and waking waiters */
    atomic_int detached;          /* 0/1 */
    atomic_int cancel_req;        /* 0/1 */
    cleanup_node *cleanup_stack;
    struct thread_info *next;
} thread_info;

/* --- global state --- */

static thread_info *g_threads = NULL;
static atomic_flag g_lock = ATOMIC_FLAG_INIT; /* spin lock */
static int reaper_pipe[2] = {-1, -1};
static atomic_int reaper_created = 0;

/* --- helpers --- */

static void lock_global(void) {
    while (atomic_flag_test_and_set(&g_lock)) { /* spin */ }
}
static void unlock_global(void) {
    atomic_flag_clear(&g_lock);
}

static int futex_wait(int *addr, int val) {
    return syscall(SYS_futex, addr, FUTEX_WAIT, val, NULL, NULL, 0);
}
static int futex_wake(int *addr, int n) {
    return syscall(SYS_futex, addr, FUTEX_WAKE, n, NULL, NULL, 0);
}

static unsigned long gettid_ul(void) {
    return (unsigned long)syscall(SYS_gettid);
}

static thread_info *find_threadinfo_by_tid(unsigned long tid) {
    thread_info *p = NULL;
    lock_global();
    for (p = g_threads; p; p = p->next) {
        if (p->tid == tid) {
            break;
        }
    }
    unlock_global();
    return p;
}

static void remove_threadinfo(thread_info *ti) {
    lock_global();
    thread_info **pp = &g_threads;
    while (*pp) {
        if (*pp == ti) {
            *pp = ti->next;
            break;
        }
        pp = &((*pp)->next);
    }
    unlock_global();
}

static int reaper_func(void *arg) {
    while (1) {
        unsigned long tid;
        ssize_t r = read(reaper_pipe[0], &tid, sizeof(tid));
        if (r <= 0) {
            break;
        }

        thread_info *ti = find_threadinfo_by_tid(tid);
        if (!ti) continue;

        remove_threadinfo(ti);
        if (ti->stack_base && ti->stack_total_size) {
            munmap(ti->stack_base, ti->stack_total_size);
        }
        
        cleanup_node *cn = ti->cleanup_stack;
        while (cn) {
            cleanup_node *n = cn->next;
            free(cn);
            cn = n;
        }
        free(ti);
    }
    return 0;
}

static void ensure_reaper_created(void) {
    if (atomic_load(&reaper_created)) return;
    lock_global();
    if (!atomic_load(&reaper_created)) {
        if (pipe(reaper_pipe) == -1) {
            perror("pipe()");
            reaper_pipe[0] = reaper_pipe[1] = -1;
        } else {
            size_t page = sysconf(_SC_PAGESIZE);
            size_t total = STACK_SIZE + page;
            void *m = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            if (m == MAP_FAILED) {
                perror("mmap for reaper stack failed");
                close(reaper_pipe[0]); close(reaper_pipe[1]);
                reaper_pipe[0] = reaper_pipe[1] = -1;
            } else {
                if (mprotect(m, page, PROT_NONE) != 0) {
                    perror("mprotect guard page");
                    munmap(m, total);
                } else {
                    void *stack_top = (char*)m + total;
                    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;
                    int rt = clone(reaper_func, stack_top, flags | SIGCHLD, NULL);
                    if (rt == -1) {
                        perror("clone reaper failed");
                        munmap(m, total);
                        close(reaper_pipe[0]); close(reaper_pipe[1]);
                        reaper_pipe[0] = reaper_pipe[1] = -1;
                    } else {
                        atomic_store(&reaper_created, 1);
                    }
                }
            }
        }
    }
    unlock_global();
}

/* push tid to reaper pipe (if available) */
static void notify_reaper(unsigned long tid) {
    if (reaper_pipe[1] == -1) return;
    ssize_t w = write(reaper_pipe[1], &tid, sizeof(tid));
    (void)w;
}

/* invoke and free all cleanup handlers in LIFO order */
static void run_all_cleanup(thread_info *ti) {
    cleanup_node *cn = ti->cleanup_stack;
    while (cn) {
        cleanup_node *n = cn->next;
        if (cn->rtn) cn->rtn(cn->arg);
        free(cn);
        cn = n;
    }
    ti->cleanup_stack = NULL;
}

/* wrapper executed as thread start */
static int thread_start_wrapper(void *args) {
  void* (*start_routine)(void*) = ((void**)(args))[0];
  void* routineArg = ((void**)(args))[1];
  free(args);
  void *ret = start_routine(routineArg);
  mythread_exit(ret);
  
  syscall(SYS_exit, 0);
  return 0;
}

/* --- API visible functions --- */

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg) {
    if (!thread || !start_routine) {
        errno = EINVAL;
        return EXIT_FAILURE;
    }

    ensure_reaper_created();

    size_t page = sysconf(_SC_PAGESIZE);
    size_t total = STACK_SIZE + page;

    void *m = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (m == MAP_FAILED) {
        perror("mmap()");
        return EXIT_FAILURE;
    }
    
    if (mprotect(m, page, PROT_NONE) != 0) {
        perror("mprotect guard page");
        munmap(m, total);
        return EXIT_FAILURE;
    }
    void *stack_top = (char*)m + total;

    void **funcArgs = malloc(2 * sizeof(void*));
    if (!funcArgs) {
        perror("malloc funcArgs");
        munmap(m, total);
        return EXIT_FAILURE;
    }
    funcArgs[0] = start_routine;
    funcArgs[1] = arg;
    
    thread_info *ti = malloc(sizeof(thread_info));
    if (!ti) {
        perror("malloc thread_info");
        free(funcArgs);
        munmap(m, total);
        return EXIT_FAILURE;
    }

    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;
    int child_tid = clone(thread_start_wrapper, stack_top, flags, funcArgs);
    if (child_tid == -1) {
        perror("clone()");
        free(funcArgs);
        munmap(m, total);
        return EXIT_FAILURE;
    }
    
    ti->tid = (unsigned long)child_tid;
    ti->stack_base = m;
    ti->stack_total_size = total;
    atomic_store(&ti->finished, 0);
    ti->retval = NULL;
    atomic_store(&ti->futex, 0);
    atomic_store(&ti->detached, 0);
    atomic_store(&ti->cancel_req, 0);
    ti->cleanup_stack = NULL;

    lock_global();
    ti->next = g_threads;
    g_threads = ti;
    unlock_global();

    *thread = ti->tid;
    return EXIT_SUCCESS;
}

void mythread_exit(void *retval) {
    unsigned long tid = gettid_ul();
    thread_info *ti = find_threadinfo_by_tid(tid);
    if (!ti) {
        syscall(SYS_exit, 0);
        return;
    }

    run_all_cleanup(ti);

    ti->retval = retval;
    atomic_store(&ti->finished, 1);
    atomic_store(&ti->futex, 1);
    futex_wake((int *)(&ti->futex), 1);

    if (atomic_load(&ti->detached)) {
        notify_reaper(ti->tid);
    }
    
    syscall(SYS_exit, 0);
}

mythread_t mythread_self(void) {
    mythread_t mt;
    mt = gettid_ul();
    return mt;
}

int mythread_equal(mythread_t t1, mythread_t t2) {
    return (t1 == t2) ? 1 : 0;
}

/* mythread_join: wait for thread termination and optionally get retval */
int mythread_join(mythread_t thread, void **retval) {
    if (thread == 0) {
        errno = EINVAL;
        return -1;
    }
    unsigned long self = mythread_self();
    if (self == thread) {
        errno = EDEADLK;
        return -1;
    }
    thread_info *ti = find_threadinfo_by_tid(thread);
    if (!ti) {
        errno = ESRCH;
        return -1;
    }
    if (atomic_load(&ti->detached)) {
        errno = EINVAL;
        return -1;
    }
    /* wait until finished */
    while (!atomic_load(&ti->finished)) {
        int v = atomic_load(&ti->futex);
        if (v == 0) {
            /* wait */
            futex_wait((int *)(&ti->futex), 0);
        } else {
            break;
        }
    }
    if (retval) *retval = ti->retval;
    
    remove_threadinfo(ti);
    if (ti->stack_base && ti->stack_total_size) {
        munmap(ti->stack_base, ti->stack_total_size);
    }
    
    cleanup_node *cn = ti->cleanup_stack;
    while (cn) {
        cleanup_node *n = cn->next;
        free(cn);
        cn = n;
    }
    free(ti);
    return 0;
}

/* mythread_detach: mark thread as detached; resources will be released on exit by reaper or immediately if already finished */
int mythread_detach(mythread_t thread) {
    thread_info *ti = find_threadinfo_by_tid(thread);
    if (!ti) {
        errno = ESRCH;
        return -1;
    }
    int already_detached = atomic_exchange(&ti->detached, 1);
    if (already_detached) {
        errno = EINVAL;
        return -1;
    }
    if (atomic_load(&ti->finished)) {
        notify_reaper(ti->tid);
    }
    return 0;
}

/* mythread_cancel: request cancellation of a thread (cooperative) */
int mythread_cancel(mythread_t thread) {
    thread_info *ti = find_threadinfo_by_tid(thread);
    if (!ti) {
        errno = ESRCH;
        return -1;
    }
    atomic_store(&ti->cancel_req, 1);
    
    if (mythread_self() == thread) {
        mythread_testcancel();
    }
    return 0;
}

/* mythread_testcancel: if cancellation requested for current thread, run cleanup and exit with PTHREAD_CANCELED */
void mythread_testcancel(void) {
    unsigned long tid = mythread_self();
    thread_info *ti = find_threadinfo_by_tid(tid);
    if (!ti) return;
    if (atomic_load(&ti->cancel_req)) {
        /* run cleanup handlers and exit with PTHREAD_CANCELED */
        run_all_cleanup(ti);
        ti->retval = (void*)PTHREAD_CANCELED;
        atomic_store(&ti->finished, 1);
        atomic_store(&ti->futex, 1);
        futex_wake((int *)(&ti->futex), INT_MAX);
        if (atomic_load(&ti->detached)) {
            notify_reaper(ti->tid);
        }
        syscall(SYS_exit, 0);
    }
}

/* mythread_cleanup_push: push handler on current thread's cleanup stack */
void mythread_cleanup_push(void (*rtn)(void *), void *arg) {
    unsigned long tid = gettid_ul();
    thread_info *ti = find_threadinfo_by_tid(tid);
    if (!ti) return;

    cleanup_node *cn = malloc(sizeof(cleanup_node));
    if (!cn) return;

    cn->rtn = rtn;
    cn->arg = arg;

    lock_global();
    cn->next = ti->cleanup_stack;
    ti->cleanup_stack = cn;
    unlock_global();
}

/* mythread_cleanup_pop: pop handler; if execute != 0, call the handler */
void mythread_cleanup_pop(int execute) {
    unsigned long tid = gettid_ul();
    thread_info *ti = find_threadinfo_by_tid(tid);
    if (!ti) return;

    lock_global();
    cleanup_node *cn = ti->cleanup_stack;
    if (!cn) {
        unlock_global();
        return;
    }
    ti->cleanup_stack = cn->next;
    unlock_global();

    if (execute && cn->rtn) cn->rtn(cn->arg);
    free(cn);
}