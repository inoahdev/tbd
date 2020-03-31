CC=clang
OBJ=obj
SRC=src

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(foreach obj,$(SRCS:src/%=%),$(OBJ)/$(basename $(obj)).o)

CFLAGS=-Iinclude/ -Wshadow -Wwrite-strings -Wunused-parameter -Wall -std=gnu11
CFLAGS+=-funroll-loops -Ofast
DEBUGCFLAGS=$(CFLAGS) -g3
RELEASECFLAGS=$(CFLAGS) -Ofast

TARGET=bin/tbd

DEBUGTARGET=bin/tbd_debug
DEBUGOBJS=$(foreach obj,$(SRCS:src/%=%),$(OBJ)/$(basename $(obj)).d.o)

.PHONY: all clean debug

$(TARGET): $(OBJS)
	@mkdir -p $(dir $(TARGET))
	@$(CC) $^ -o $@
 
clean:
	@$(RM) -rf $(OBJ)
	@$(RM) -rf $(dir $(TARGET))

debug: $(DEBUGTARGET)

$(DEBUGTARGET): $(DEBUGOBJS)
	@mkdir -p $(dir $(DEBUGTARGET))
	@$(CC) $^ -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(OBJ)
	@$(CC) $(RELEASECFLAGS) -c $< -o $@

$(OBJ)/%.d.o: $(SRC)/%.c
	@mkdir -p $(OBJ)
	@$(CC) $(DEBUGCFLAGS) -c $< -o $@
