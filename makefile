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
TST_DIR		= ./test
C_TST_DIR	= $(TST_DIR)/correct
I_TST_DIR	= $(TST_DIR)/incorrect
LOG_DIR		= ./log
C_LOG_DIR	= $(LOG_DIR)/correct
I_LOG_DIR	= $(LOG_DIR)/incorrect

# Tell Make which shell to use
SHELL 		= bash

# Compiler Info
CC 			= g++
CFLAGS 		= -g -Wall

TARGET		= $(BIN_DIR)/$(PROJECT)
SRC_FILES	= $(wildcard $(SRC_DIR)/*.cpp)
HDR_FILES	= $(wildcard $(SRC_DIR)/*.h)
OBJ_FILES	= $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
# Correct tests/logs
C_TST_FILES	= $(wildcard $(C_TST_DIR)/*.src)
C_LOG_FILES = $(patsubst $(C_TST_DIR)/%.src, $(C_LOG_DIR)/%.log, $(C_TST_FILES))
# Incorrect tests
I_TST_FILES	= $(wildcard $(I_TST_DIR)/*.src)
I_LOG_FILES = $(patsubst $(I_TST_DIR)/%.src, $(I_LOG_DIR)/%.log, $(I_TST_FILES))

# Build Targets
.PHONY: clean

$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

all: $(TARGET) test

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR) $(OBJ_DIR) $(LOG_DIR) $(C_LOG_DIR) $(I_LOG_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(LOG_DIR)

# Test files
# Use pattern rule from the big makefile example Tyler gave me
# Something like:
# $(LOG_DIR)/correct/%.log: $(TST_DIR)/correct/%.src $(TARGET)
# 	Call $(TARGET) on the stuff and things
test: $(C_LOG_FILES) $(I_LOG_FILES)

$(C_LOG_DIR)/%.log: $(C_TST_DIR)/%.src $(TARGET) | $(C_LOG_DIR)
	$(TARGET) -w -v 2 -l $@ -i $<

$(I_LOG_DIR)/%.log: $(I_TST_DIR)/%.src $(TARGET) | $(I_LOG_DIR)
	$(TARGET) -w -v 2 -l $@ -i $<

