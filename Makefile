SHELL = /bin/sh
C := clang

WARNINGFLAGS := -Wshadow
WARNINGFLAGS := $(WARNINGFLAGS) -Wwrite-strings -Wunused-parameter

DEFAULTFLAGS := -std=gnu11 -I. -Iinclude/ $(WARNINGFLAGS)
CFLAGS := $(DEFAULTFLAGS) -Ofast -funroll-loops
VSCODEFLAGS := -I.vscode/ -Wno-unused-parameter -Wno-sign-conversion $(CFLAGS)

SRCS := $(shell find src -name "*.c")
TARGET := bin/tbd

EXTRADEBUGFLAGS := -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer
DEBUGFLAGS := $(DEFAULTFLAGS) -g $(EXTRADEBUGFLAGS)

.DEFAULT_GOAL := all

clean:
	@$(RM) $(TARGET)

target-dir:
	@mkdir -p $(dir $(TARGET))

all: target-dir
	@$(C) $(CFLAGS) $(SRCS) -o $(TARGET)

debug: target-dir
	@$(C) $(DEBUGFLAGS) $(SRCS) -o $(TARGET)

install: all
	@sudo mv $(TARGET) /usr/bin

vscode:
	@$(C) $(VSCODEFLAGS) $(SRCS) -o $(TARGET)

compile_commands:
	@bear make vscode
