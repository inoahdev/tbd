SHELL = /bin/sh
C := clang

WARNINGFLAGS := -Wall -W -Wconversion -Wcast-qual -Wshadow -Wwrite-strings
DEFAULTFLAGS := -std=gnu11 -Iinclude/ $(WARNINGFLAGS)

CFLAGS := $(DEFAULTFLAGS) -Ofast -funroll-loops

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

compile_commands:
	@echo "[\n\t{\n\t\t\"command\" : \"$(C) $(CFLAGS) $(SRCS)\"\n\t}\n]" > compile_commands.json
