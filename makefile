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
TEST_DIR	= ./test
LOG_DIR		= ./log

# Tell Make which shell to use
SHELL 		= bash

# Compiler Info
CC 			= g++
CFLAGS 		= -g -Wall

TARGET		= $(BIN_DIR)/$(PROJECT)
SRC_FILES	= $(wildcard $(SRC_DIR)/*.cpp)
HDR_FILES	= $(wildcard $(SRC_DIR)/*.h)
OBJ_FILES	= $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Build Targets
.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR) $(OBJ_DIR) $(LOG_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(LOG_DIR)

# Use pattern rule from the big makefile example Tyler gave me
# Something like:
# $(LOG_DIR)/correct/%.log: $(TEST_DIR)/correct/%.src $(TARGET)
# 	Call $(TARGET) on the stuff and things

