SHELL = /bin/sh

C := clang

USERDEFINES := "-D__unused=__attribute__((unused))"
WARNINGFLAGS := -Wall -W -Wconversion -Wcast-qual -Wshadow -Wwrite-strings

DEFAULTFLAGS := -std=gnu11 $(USERDEFINES) $(WARNINGFLAGS)
CFLAGS := $(DEFAULTFLAGS) -Ofast -Iinclude/

SRCS := $(shell find src -name "*.c")
TARGET := bin/tbd

DEBUGFLAGS := -std=gnu11 -Wall -g -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -Iinclude/
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
