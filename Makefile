SHELL = /bin/sh

CXX := clang++
CXXFLAGS := -std=c++17 -stdlib=libc++ -Wall -O3

SRCS := $(shell find src -name "*.cc")
TARGET := build/tbd

DEBUGFLAGS := -std=c++17 -stdlib=libc++ -Wall -g

all:
	@$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

debug:
	@$(CXX) $(DEBUGFLAGS) $(SRCS) -o $(TARGET)

clean:
	@$(RM) $(TARGET)

install: all
	@sudo mv $(TARGET) /usr/bin
