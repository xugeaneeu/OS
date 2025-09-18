#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    void *ptr = malloc(1024 * 1024 * 10);
    if (ptr == NULL) {
        perror("malloc");
        return 1;
    }

    memset(ptr, 0, 1024 * 1024 * 10);
    printf("PID: %d\nAllocated to address: %p\n", getpid(), ptr);
    sleep(60);
    free(ptr);
    return 0;
}