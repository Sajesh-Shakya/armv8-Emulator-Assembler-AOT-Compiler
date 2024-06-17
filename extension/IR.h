#ifndef IR_H
#define IR_H

#include <stdio.h>
#include <stdint.h>

#define MAX_DIR 128
#define MAX_NAME 16
#define MAX_VAR 64
#define MAX_FUNC 16
#define MAX_INSTR 256

typedef enum {
    IR_ADD,
    IR_SUB,
    IR_CMP,
    IR_NEG,
    IR_MUL, // mul, mneg, madd, msub
    IR_DIV,
    IR_MOD,
    IR_AND,
    IR_BIC,
    IR_OR,
    IR_EOR,
    IR_TST,
    IR_WIDEMOVE,
    IR_MOV,

    IR_B,
    IR_BC,
    IR_BR,

    IR_LDR,
    IR_STR,

    IR_LABEL,
    IR_DIR
} IRType;

typedef struct IRInstruction {
    IRType type;
    int dest;
    int src1;
    int src2;   
    int src3;
    int line;
    int count;
    struct IRInstruction *next;
} IRInstruction;

typedef struct {
    char name[MAX_NAME];
    int64_t value;
} Entry;

typedef struct IRProgram {
    IRInstruction *head;
    IRInstruction *tail;
    Entry map[MAX_VAR];
    Entry funcs[MAX_FUNC];
    int64_t directives[MAX_DIR];
    uint32_t instructions[MAX_INSTR];
} IRProgram;

#endif