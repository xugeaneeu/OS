.ONESHELL:

CC      := clang
					 
CFLAGS     := -pthread
BUILD_DIR  := build

SRCS    := src/main.c src/proxy.c src/cache.c
OBJS    := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET  := $(BUILD_DIR)/proxy

TEST_DIR      := tests
TEST_SCRIPTS  := $(wildcard $(TEST_DIR)/*.sh)

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run-tests: all
	chmod +x $(TEST_SCRIPTS)
	@echo ">>> Starting proxy in background"
	$(TARGET) &
	@PROXY_PID=$$!; \
	echo ">>> Proxy PID=$$PROXY_PID"; \
	sleep 1; \
	for test in $(TEST_SCRIPTS); do \
	  echo; \
	  echo "=== RUN $$test ==="; \
	  $$test || { echo "*** Test $$test FAILED ***"; kill $$PROXY_PID; exit 1; }; \
	  echo "=== PASS $$test ==="; \
	done; \
	echo; \
	sleep 10
	echo ">>> All tests passed!"; \
	echo ">>> Shutting down proxy"; \
	kill $$PROXY_PID; \
	wait $$PROXY_PID; \
	rm 200mb.dat; \
	rm -rf results; \
	make clean; \
	exit 0

clean:
	rm -rf $(BUILD_DIR)
