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
TESTDIR	= ./test
LOGDIR	= ./log

# Tell Make which shell to use
SHELL 	= bash

# Compiler Info
CC 		= g++
CFLAGS 	= -g -Wall

TARGET	= $(BINDIR)/$(PROJECT)
CPP_SRC	= $(addprefix $(SRCDIR)/, log.cpp main.cpp scanner.cpp token.cpp)
H_SRC	= $(addprefix $(SRCDIR)/, scanner.h log.h token.h)
OBJS	= $(addprefix $(OBJDIR)/, log.o main.o scanner.o token.o)
DIRS	= $(BINDIR) $(LOGDIR) $(OBJDIR)

# Build Targets
.PHONY: clean

all: $(DIRS) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

obj/log.o:
	$(CC) $(CFLAGS) -c -o obj/log.o src/log.cpp

obj/main.o:
	$(CC) $(CFLAGS) -c -o obj/main.o src/main.cpp

obj/token.o:
	$(CC) $(CFLAGS) -c -o obj/token.o src/token.cpp

obj/scanner.o:
	$(CC) $(CFLAGS) -c -o obj/scanner.o src/scanner.cpp

$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(DIRS)

# Use pattern rule from the big makefile example Tyler gave me
# Something like:
# $(LOGDIR)/correct/%.log: $(TESTDIR)/correct/%.src $(TARGET)
# 	Call $(TARGET) on the stuff and things

