CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11

all: symtable.o

symtable.o : symtable.c symtable.h
	$(CC) $(CFLAGS) -c $< -o $@