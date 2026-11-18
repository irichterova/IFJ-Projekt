CC = gcc
CFLAGS = -g -Wall -Wextra -std=c11

all: ifj26

ifj26: main.o ast.o buffer.o error.o expression.o generator.o parser.o scanner.o semantic.o stack.o symtable.o
	$(CC) $(CFLAGS) $^ -o ifj26

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c $< -o $@

buffer.o: buffer.c buffer.h
	$(CC) $(CFLAGS) -c $< -o $@

error.o: error.c error.h
	$(CC) $(CFLAGS) -c $< -o $@

expression.o: expression.c expression.h ast.h scanner.h error.h
	$(CC) $(CFLAGS) -c $< -o $@

generator.o: generator.c generator.h ast.h
	$(CC) $(CFLAGS) -c $< -o $@

parser.o: parser.c parser.h ast.h error.h scanner.h expression.h
	$(CC) $(CFLAGS) -c $< -o $@

scanner.o: scanner.c scanner.h stack.h buffer.h error.h
	$(CC) $(CFLAGS) -c $< -o $@

semantic.o: semantic.c semantic.h ast.h error.h symtable.h
	$(CC) $(CFLAGS) -c $< -o $@

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c $< -o $@

symtable.o: symtable.c symtable.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o ifj26