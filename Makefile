PROJECT_NAME ?= main
CFLAGS := -g -Wall -O
CXXFLAGS := $(CFLAGS) 
CC ?= gcc

TARGET_DIR = build

subdirs := $(shell find . -type d)
SRCS += $(foreach dir,$(subdirs),$(wildcard $(dir)/*.c))

OBJS = $(sort $(SRCS:%.c=$(TARGET_DIR)/%.o))

TARGET_BIN := $(TARGET_DIR)/$(PROJECT_NAME)

$(TARGET_BIN): $(OBJS)
	$(CC) $^ -o $@

$(TARGET_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET_BIN)
