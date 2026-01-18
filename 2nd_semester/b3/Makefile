CC      := clang
					 
CFLAGS := -pthread
BUILD_DIR := build

SRCS    := src/main.c src/proxy.c src/cache.c
OBJS    := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET  := $(BUILD_DIR)/proxy

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)