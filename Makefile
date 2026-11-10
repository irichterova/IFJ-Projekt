CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11

all: symtable.o expression.o

symtable.o : symtable.c symtable.h
	$(CC) $(CFLAGS) -c $< -o $@

expression.o : expression.c expression.h ast.h scanner.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o

.PHONY: all clean
