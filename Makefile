CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11

.PHONY: all run clean

all: run

expr: expression.o

run: generator.exe
	generator.exe > vystup.code
	type vystup.code

generator.exe: generator.o
	$(CC) $(CFLAGS) generator.o -o generator.exe

generator.o: generator.c generator.h
	$(CC) $(CFLAGS) -c generator.c -o generator.o

expression.o : expression.c expression.h ast.h scanner.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /f generator.exe generator.o vystup.code 2>nul || exit 0

clean-all:
	rm -f *.o

clean-expr:
	rm -f expression.o