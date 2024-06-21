#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "ir.h"
#include "ast.h"
#include "../src/io.h"

#define MAX_TOKEN_LENGTH 20


#define GENERATE_STRING(enum_type) #enum_type

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
    token->reg = value;
    return token;
}

IRInstruction *create_ir_instruction(IRType type, int dest, int src1, int src2, int src3, int *line)
{
    IRInstruction* instruction = malloc(sizeof(IRInstruction));
    assert(instruction != NULL);
    instruction->type = type;
    instruction->dest = (dest != NOT_USED) ? create_token(dest) : NULL;
    instruction->src1 = (src1 != NOT_USED) ? create_token(src1) : NULL;
    instruction->src2 = (src2 != NOT_USED) ? create_token(src2) : NULL;
    instruction->src3 = (src3 != NOT_USED) ? create_token(src3) : NULL;
    instruction->line = (*line)++;
    instruction->count = 0;
    instruction->next = NULL;
    return instruction;
}

void free_ir_instruction(struct IRInstruction *instruction)
{
    while (instruction) {
        struct IRInstruction* next = instruction->next;
        free(instruction);
        instruction = next;
    }
}

State *create_state(void)
{
    State *state = malloc(sizeof(State));
    assert(state != NULL);

    state->map_size = 0;
    state->funcs_size = 0;
    state->stack_size = (rand() % (MEMORY_SIZE_2 - STACK_OFFSET)) / 64 * 64 + STACK_OFFSET;
    state->hotspots_size = 0;

    return state;
}

void free_state(State *state)
{
    free(state);
}

// HotMap hotspots[MAX_HOTSPOTS];
// int hotspots_count;

// Functions to work with hotspots

void putMnemonic(char *assembly_line, IRType type)
{
    char mnemonic[MAX_TOKEN_LENGTH];
    switch(type) {
        case IR_ADD: strcpy(mnemonic, "add"); break;
        case IR_SUB: strcpy(mnemonic, "sub"); break;
        case IR_CMP: strcpy(mnemonic, "cmp"); break;
        case IR_NEG: strcpy(mnemonic, "neg"); break;
        case IR_AND: strcpy(mnemonic, "and"); break;
        case IR_EOR: strcpy(mnemonic, "eor"); break;
        case IR_ORR: strcpy(mnemonic, "orr"); break;
        case IR_TST: strcpy(mnemonic, "tst"); break;
        case IR_MOV: strcpy(mnemonic, "mov"); break;
        case IR_MOVZ: strcpy(mnemonic, "movz"); break;
        case IR_MVN: strcpy(mnemonic, "mvn"); break;
        case IR_MADD: strcpy(mnemonic, "madd"); break;
        case IR_MUL: strcpy(mnemonic, "mul"); break;
        case IR_B: strcpy(mnemonic, "b"); break;
        case IR_BR: strcpy(mnemonic, "br"); break;
        case IR_BCOND: strcpy(mnemonic, "b.cond"); break;
        case IR_LDR: strcpy(mnemonic, "ldr"); break;
        case IR_STR: strcpy(mnemonic, "str"); break;
        case IR_DIR: strcpy(mnemonic, "dir"); break;
        default: strcpy(mnemonic, "unknown"); break;
    }
    strcat(assembly_line, mnemonic);
}

void putReg(char *assembly_line, int src) {
    char reg[MAX_TOKEN_LENGTH];
    sprintf(reg, "w%d", src);
    strcat(assembly_line, reg);
}

void putImm(char *assembly_line, int src) {
    char imm[MAX_TOKEN_LENGTH];
    sprintf(imm, "#%d", src);
    strcat(assembly_line, imm);
}

void putRegOrImm(char *assembly_line, Token *token)
{
     if (token->reg != NOT_USED) {
        strcat(assembly_line, " ");
        if (token->type == REG) {
            putReg(assembly_line, token->reg);
        } else if (token->type == IMM) {
            putImm(assembly_line, token->reg);
        } else {
            // I have no idea. We shouldn't assemble lines with labels
        }
    }
}

uint32_t getHotSpot(IRInstruction *ir_instr)
{
    char assembly_line[MAX_ASSEMBLY_LINE] = "";
    putMnemonic(assembly_line, ir_instr->type);
    putRegOrImm(assembly_line, ir_instr->dest);
    putRegOrImm(assembly_line, ir_instr->src1);
    putRegOrImm(assembly_line, ir_instr->src2);
    putRegOrImm(assembly_line, ir_instr->src3);

    // InstructionParse *instr = initializeInstructionParse();
    // Instruction *instruction = initializeInstruction();

    // // Decompose
    // char *instrSavePntr = NULL;
    // char *token = strtok_r(instr->buff, SPACE, &instrSavePntr);
    // strcpy(instr->instrname, token);
    // instr->type = identifyType(instr->instrname);
    // instr->numTokens = 0;
    // token = strtok_r(NULL, SPACECOMMA, &instrSavePntr);
    // while (token != NULL) {
    //     strcpy(instr->tokens[instr->numTokens++], token);
    //     token = strtok_r(NULL, SPACECOMMA, &instrSavePntr);
    // }

    // // Disassemble
    // int disassembleError = disassemble(instr, instruction);
    // checkError(disassembleError);
    
    // // Decode
    // uint32_t instrBin = 0;
    // int decodeError = decode(&instrBin, instruction, putBits);
    // checkError(decodeError);

    // Return
    return 0;
}

void addHotSpot(State *state, uint32_t instrBin, int line)
{
    state->hotspots[state->hotspots_size].instruction = instrBin;
    state->hotspots[state->hotspots_size++].line = line;
}

uint32_t returnHotSpot(State *state, int line)
{
    for (int i = 0; i < state->hotspots_size; i++) {
        if (state->hotspots[i].line == line) {
            return state->hotspots[i].instruction;
        }
    }
    exit(EXIT_FAILURE);
}


// Executes the program and counts the number of times each line is executed
void executeProgram(IRProgram *program)
{
    IRInstruction *instr = program->head;
    while (instr != NULL) {
        // Need to support IR_BR as well, not easy to do
        instr->count++;
        instr = instr->next;
    }
}

