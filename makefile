# File			: Makefile
# Author		: Wayne Stegner <wayne.stegner@protonmail.com>

# Project Name
PROJECT = compiler

# Directory Definitions
# src/		- Source code directory
# bin/		- Executables are built here
# obj/		- Intermediate object files are built here
SRCDIR		= ./src
OBJDIR		= ./obj
BINDIR		= ./bin
LOGDIR		= ./log

# Tell Make which shell to use
SHELL 		= bash

# Compiler Info
CC 			= g++
CFLAGS 		= -g -Wall

TARGET	= $(BINDIR)/$(PROJECT)
OBJS	= $(addprefix $(OBJDIR)/, main.o log.o)
DIRS	= $(SRCDIR) $(OBJDIR) $(BINDIR) $(LOGDIR)

# Build Targets
.PHONY: clean

all: $(DIRS) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(LOGDIR)

