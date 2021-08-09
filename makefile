# File			: Makefile
# Author		: Wayne Stegner <wayne.stegner@protonmail.com>

# Project Name
PROJECT = compiler

# Directory Definitions
# src/	- Source code directory
# obj/	- Compiled object directory
# bin/	- Compiled executable directory
# test/	- Test case directory
# log/	- Test log directory
SRC_DIR		= ./src
OBJ_DIR		= ./obj
BIN_DIR		= ./bin
C_BIN_DIR	= $(BIN_DIR)/correct
I_BIN_DIR	= $(BIN_DIR)/incorrect
TST_DIR		= ./test
C_TST_DIR	= $(TST_DIR)/correct
I_TST_DIR	= $(TST_DIR)/incorrect
LOG_DIR		= ./log
C_LOG_DIR	= $(LOG_DIR)/correct
I_LOG_DIR	= $(LOG_DIR)/incorrect

# Tell Make which shell to use
SHELL		= bash

# Compiler Info
CC			= g++
CFLAGS		= -g -Wall

TARGET		= $(BIN_DIR)/$(PROJECT)
RUNTIME_C	= $(SRC_DIR)/runtime/runtime.c
RUNTIME_O	= $(BIN_DIR)/runtime.o
RUNTIME_S	= $(BIN_DIR)/runtime.so
SRC_FILES	= $(wildcard $(SRC_DIR)/*.cpp)
HDR_FILES	= $(wildcard $(SRC_DIR)/*.h)
OBJ_FILES	= $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
# Correct tests/logs
C_TST_FILES	= $(wildcard $(C_TST_DIR)/*.src)
C_LOG_FILES	= $(patsubst $(C_TST_DIR)/%.src, $(C_LOG_DIR)/%.log, $(C_TST_FILES))
C_LL_FILES	= $(patsubst $(C_TST_DIR)/%.src, $(C_BIN_DIR)/%.ll, $(C_TST_FILES))
C_BC_FILES	= $(patsubst $(C_TST_DIR)/%.src, $(C_BIN_DIR)/%.bc, $(C_TST_FILES))
# Incorrect tests
I_TST_FILES	= $(wildcard $(I_TST_DIR)/*.src)
I_LOG_FILES	= $(patsubst $(I_TST_DIR)/%.src, $(I_LOG_DIR)/%.log, $(I_TST_FILES))
I_LL_FILES	= $(patsubst $(I_TST_DIR)/%.src, $(I_BIN_DIR)/%.ll, $(I_TST_FILES))

# Build Targets
.PHONY: clean all clean_all
.PRECIOUS: $(C_LL_FILES) $(I_LL_FILES)

$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

all: $(TARGET) $(RUNTIME_S) test

$(RUNTIME_S): $(RUNTIME_O) | $(BIN_DIR)
	gcc -shared -o $@ $<

$(RUNTIME_O): $(RUNTIME_C) | $(BIN_DIR)
	gcc -c -Wall -Werror -fpic -o $@ $<

clean_all: clean all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR) $(OBJ_DIR) $(LOG_DIR) $(C_LOG_DIR) $(I_LOG_DIR) $(C_BIN_DIR) $(I_BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(LOG_DIR)

# Test files
# Use pattern rule from the big makefile example Tyler gave me
# Something like:
# $(LOG_DIR)/correct/%.log: $(TST_DIR)/correct/%.src $(TARGET)
# 	Call $(TARGET) on the stuff and things
test: $(C_BC_FILES) $(I_LL_FILES)

$(C_BIN_DIR)/%.bc: $(C_BIN_DIR)/%.ll | $(C_BIN_DIR)
	-llvm-as $<

$(C_BIN_DIR)/%.ll: $(C_TST_DIR)/%.src $(TARGET) | $(C_BIN_DIR)
	-$(TARGET) -w -v 2 -i $< -o $@

$(I_BIN_DIR)/%.ll: $(I_TST_DIR)/%.src $(TARGET) | $(I_BIN_DIR)
	-$(TARGET) -w -v 2 -i $< -o $@
