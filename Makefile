SHELL = /bin/sh

CXX := clang++
CXXFLAGS := -std=c++17 -stdlib=libc++ -Wall -O3

SRCS := $(shell find src -name "*.cc")
TARGET := build/tbd

DEBUGFLAGS := -std=c++17 -stdlib=libc++ -Wall -g

.DEFAULT_GOAL := all

clean:
	@$(RM) $(TARGET)

build-dir:
	@mkdir -p build

all: build-dir
	@$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

debug: build-dir
	@$(CXX) $(DEBUGFLAGS) $(SRCS) -o $(TARGET)

install: all
	@sudo mv $(TARGET) /usr/bin
