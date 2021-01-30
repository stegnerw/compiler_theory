# File			: Makefile
# Author		: Wayne Stegner <wayne.stegner@protonmail.com>

# Project Name
PROJECT = compiler

# Directory Definitions
# src/	- Source code directory
# bin/	- Executables are built here
# obj/	- Intermediate object files are built here
SRCDIR	= ./src
OBJDIR	= ./obj
BINDIR	= ./bin
LOGDIR	= ./log

# Tell Make which shell to use
SHELL 	= bash

# Compiler Info
CC 		= g++
CFLAGS 	= -g -Wall

TARGET	= $(BINDIR)/$(PROJECT)
CPP_SRC	= $(addprefix $(SRCDIR)/, log.cpp main.cpp scanner.cpp token.cpp)
H_SRC	= $(addprefix $(SRCDIR)/, scanner.h log.h token.h)
DIRS	= $(BINDIR) $(LOGDIR)

# Build Targets
.PHONY: clean

all: $(DIRS) $(TARGET)

$(TARGET): $(CPP_SRC) $(H_SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(CPP_SRC)

$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(DIRS)

