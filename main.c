/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file main.c
 */

#include "generator.h"
#include "parser.h"
#include "semantic.h"

int main(void) {
    program_struct *program = parse_program();
    check_program(program);
    gen_code(program);
    free_program(program);
    return 0;
}