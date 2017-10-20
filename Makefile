SHELL = /bin/sh

CXX := clang++
CXXFLAGS := -std=c++1z -stdlib=libc++ -Wall -O3

SRCS := $(shell find src -name "*.cc")
TARGET := build/tbd

DEBUGFLAGS := -std=c++1z -stdlib=libc++ -Wall -g

.DEFAULT_GOAL := all

clean:
	@$(RM) $(TARGET)

target-dir:
	@mkdir -p $(dir $(TARGET))

all: target-dir
	@$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

debug: target-dir
	@$(CXX) $(DEBUGFLAGS) $(SRCS) -o $(TARGET)

install: all
	@sudo mv $(TARGET) /usr/bin
