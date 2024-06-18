#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "ir.h"

IRProgram* create_ir_program(void)
{
    IRProgram *program = malloc(sizeof(IRProgram));
    assert(program != NULL);
    program->head = NULL;
    program->tail = NULL;
    return program;
}

void free_ir_program(IRProgram *program)
{
    if (program) {
        free_ir_instruction(program->head);
        free(program);
    }
}

Token *create_token(uint8_t value)
{
    Token *token = malloc(sizeof(Token));
    assert(token != NULL);
    return token;
}

IRInstruction *create_ir_instruction(IRType type, int dest, int src1, int src2, int src3, int *line)
{
    IRInstruction* instruction = malloc(sizeof(IRInstruction));
    assert(instruction != NULL);
    instruction->type = type;
    instruction->dest = create_token(dest);
    instruction->src1 = create_token(src1);
    instruction->src2 = create_token(src2);
    instruction->src3 = create_token(src3);
    instruction->line = (*line)++;
    instruction->count = 0;
    instruction->next = NULL;
    return instruction;
}

void free_ir_instruction(IRInstruction *instruction)
{
    while (instruction) {
        IRInstruction* next = instruction->next;
        free(instruction);
        instruction = next;
    }
}


// TODO: Flow return_address figuring out
// TODO: Augmented Assign
// TODO: more putBits
// TODO: jit.c