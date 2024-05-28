#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "disassembler.h"

// Global keywords
const char *singleOperand[] = {
"mov", "mul", "mneg", "neg", "negs", "mvn", "movz", "movn", "movk"};
const char *twoOperandNoDest[] = {
"cmp", "cmn", "tst"};
const char *arithmetics[] = {
    "add", "adds", "sub", "subs"};
const char *logical[] = {
    "and", "bic", "orr", "orn", "eor", "eon", "ands", "bics"};
const char *shifts[] = {
    "lsl", "lsr", "asr", "ror"};
const char *aliases[] = {
    "cmp", "cmn", "neg", "negs", "tst", "mvn", "mov", "mul", "mneg"};


// 2.3.1 Data Processing Instructions
void disassembleDP(char *instrname, char **tokens, int numTokens, FILE *outputFile)
{
    // Start with xxxx...
    int instruction = 0;

    // Set sf
    instruction |= getMode(tokens[0]) << 31;

    // Broke down depending on the opcode

    // Multiply - <mul_opcode> rd, rn, rm, ra
    // Check that 4th token is a register
    if (numTokens == 4 && strchr(tokens[3], '#') == NULL)
    {
        // tokens: 0 - rd, 1 - rn, 2 - rm, 3 - ra

        // Set 1s
        instruction |= (1 << 28) + (1 << 27) + (1 << 25) + (1 << 24);
        // Set rm
        instruction |= getRegister(tokens[2]) << 16;
        // Set x: 0 - madd, 1 - msub
        instruction |= !strcmp(instrname, "msub") << 15;
        // Set ra
        instruction |= getRegister(tokens[3]) << 10;
        // Set rn
        instruction |= getRegister(tokens[1]) << 5;
        // Set rd
        instruction |= getRegister(tokens[0]);

        // instr[PC++] = instruction;
        fwrite(&instruction, sizeof(int), 1, outputFile);
        return;
    }

    // Two operands, no destination - <opcode> rn, <operand> - cmp, cmn, tst
    bool isTwoOpNoDest = 0;
    for (int i = 0; i < 3; i++)
    {
        if (!strcmp(instrname, twoOperandNoDest[i]))
            isTwoOpNoDest = 1;
    }
    if (isTwoOpNoDest)
    {
        // These are all aliases
        disassembleAliases(instrname, tokens, numTokens, outputFile);
        return;
    }

    // Single operand - <opcode> rd, <operand> - mov, mul, mneg, neg(s), mvn, wide-moves
    short isSingleOperand = -1;
    for (int i = 0; i < 9; i++)
    {
        if (!strcmp(instrname, singleOperand[i]))
            isSingleOperand = i;
    }
    if (isSingleOperand != -1)
    {
        if (isSingleOperand < 6)
        {
            // These are all aliases
            disassembleAliases(instrname, tokens, numTokens, outputFile);
            return;
        }
        else
        {
            // These are wide moves

            // Set 1s
            instruction |= 1 << 28;
            // Set rd
            instruction |= getRegister(tokens[0]);
            // Set opi = 101
            instruction |= (1 << 25) + (1 << 23);
            // Set imm16
            instruction |= getImmediate(tokens[1]) << 5;
            // Set hw
            if (numTokens > 2) // lsl #<imm> case
                instruction |= (getImmediate(tokens[3]) / 16) << 21;
            // else hw = 0

            // Set opc
            // Cases: movn, movk, movz
            if (strcmp(instrname, "movn")) // movz, movk cases
            {
                // Set opc to 10
                instruction |= 1 << 30;
                if (!strcmp(instrname, "movk"))
                    instruction |= 1 << 29;
            }
            // instr[PC++] = instruction;
            fwrite(&instruction, sizeof(int), 1, outputFile);
            return;
        }
    }

    // Two operand - <opcode> rd, rn, <operand> - arithmetic, bit-wise
    if (!strcmp(instrname, "mul") || !strcmp(instrname, "mneg"))
    {
        // These are aliases
        disassembleAliases(instrname, tokens, numTokens, outputFile);
        return;
    }

    // Next:  add, adds, sub, subs (imm + reg)
    // Next:  and, ands, bic, bics, eor, orr, eon, orn (reg)

    // Common features for both immediate and register
    // Set rd
    instruction |= getRegister(tokens[0]);
    // Set rn
    instruction |= getRegister(tokens[1]) << 5;

    // Get imm or rd
    bool rOrImm;
    int rOrImmValue = getOperand(tokens[2], &rOrImm);
    // Check for <shift> #<imm>
    int shiftType = 0;
    int shiftValue = 0;
    if (numTokens > 3) // shift exists case
    {
        shiftType = getShift(tokens[3]);
        shiftValue = getImmediate(tokens[4]);
    }

    // Specific features for immediate and register
    if (rOrImm)
    {
        // Immediate Case - Arithmetic only

        // Set 1s
        instruction |= 1 << 28;
        // Set opi = 010
        instruction |= 1 << 24;
        // Set imm12
        instruction |= rOrImmValue << 10;
        // Set sh - is 1 when we shift by 12
        instruction |= (shiftValue == 12) << 22;
        // Set opc
        for (int opc = 0; opc < 4; opc++)
        {
            if (!strcmp(instrname, arithmetics[opc]))
                instruction |= opc << 29;
        }
    }
    else
    {
        // Register Case - Arithmetic + Logic

        // Set 1s
        instruction |= (1 << 27) + (1 << 25);
        // Set rm
        instruction |= getRegister(tokens[2]) << 16;

        // arithmetic - 1, logic - 0
        bool arithmOrLogic = false;
        for (int opc = 0; opc < 4; opc++)
        {
            if (!strcmp(instrname, arithmetics[opc]))
            {
                instruction |= opc << 29;
                arithmOrLogic = true;
            }
        }

        // General for both arithmetic and logical
        // Set opr
        instruction |= arithmOrLogic << 24;
        instruction |= shiftType << 22;
        // Set operand
        instruction |= shiftValue << 10;

        // Arithmetic done
        // Specific for logical
        if (!arithmOrLogic)
        {
            // Set opc, N
            for (int opc = 0; opc < 8; opc++)
            {
                if (!strcmp(instrname, logical[opc]))
                {
                    // opc = 00, 01, 10, 11
                    instruction |= (opc / 2) << 29;
                    // N = 0 or 1
                    instruction |= (opc % 2) << 21;
                }
            }
        }
    }

    // instr[PC++] = instruction;
    fwrite(&instruction, sizeof(int), 1, outputFile);
}


// Assemble aliases
// Static because we don't call it explicitly
static void disassembleAliases (char *instrname, char **tokens, int numTokens, FILE *outputFile)
{
    // 9 cases
    // 0 - cmp, 1 - cmn, 2(3) - neg(s), 4 - tst,
    // 5 - mvn, 6 - mov, 7 - mul, 8 - mneg
    // For each: set up tokens and delegate to disassembleDP again

    // Get mode: w or x
    int mode = getMode(tokens[0]);
    int instr;
    for (instr = 0; instr < 9; instr++)
    {
        if (!strcmp(instrname, aliases[instr]))
            break;
    }
    // instr has the index of the mnemonic in aliases[]

    // Add rzr as first token
    // cmp, cmn, tst
    if (instr <= 1 || instr == 4)
    {
        for (int i = numTokens; i > 0; i--)
            strcpy(tokens[i], tokens[i - 1]);
        strcpy(tokens[0], mode ? "xzr" : "wzr");
        switch (instr)
        {
        case 0:
            strcpy(instrname, "subs");
            break;
        case 1:
            strcpy(instrname, "adds");
            break;
        case 4:
            strcpy(instrname, "ands");
            break;
        }
        disassembleDP(instrname, tokens, ++numTokens, outputFile);
        return;
    }

    // Add rzr as second token
    // neg, negs, mvn, mov
    if (instr == 2 || instr == 3 || instr == 5 || instr == 6)
    {
        for (int i = numTokens; i > 1; i--)
            strcpy(tokens[i], tokens[i - 1]);
        strcpy(tokens[1], mode ? "xzr" : "wzr");
        switch (instr)
        {
        case 2:
            strcpy(instrname, "sub");
            break;
        case 3:
            strcpy(instrname, "subs");
            break;
        case 5:
            strcpy(instrname, "orn");
            break;
        case 6:
            strcpy(instrname, "orr");
            break;
        }
        disassembleDP(instrname, tokens, ++numTokens, outputFile);
        return;
    }

    // Add rzr as last token
    // mul, mneg
    strcpy(tokens[numTokens], mode ? "xzr" : "wzr");
    switch (instr)
    {
    case 7:
        strcpy(instrname, "madd");
        break;
    case 8:
        strcpy(instrname, "msub");
        break;
    }
    disassembleDP(instrname, tokens, ++numTokens, outputFile);
}


// 2.3.2 Single Data Transfer Instructions
void disassembleLS(char *instrname, char **tokens, int numTokens, FILE *outputFile, int PC)
{
    // Start with xxx1100xxxx...
    int instruction = (1 << 28) + (1 << 27);

    // Set rt
    instruction |= getRegister(tokens[0]);
    // Set sf
    int mode = getMode(tokens[0]);
    instruction |= mode << 30;

    // Check address: register or literal
    if (*tokens[1] == '[')
    {
        // Single Data Transfer

        // Common features for all
        // Set 1s
        instruction |= (1 << 31) + (1 << 29);
        // Set L: 1 - load, 0 - store
        instruction |= (!strcmp(instrname, "ldr")) << 22;
        if (strchr(tokens[1], ']') != NULL)
            *(tokens[1] + 4) = '\0'; // remove ] if necessary
        // Set xn
        instruction |= getRegister(tokens[1] + 1) << 5; // remove [

        // Specific features for all
        // Zero Unsigned Offset
        if (numTokens == 2)
        {
            // No imm provided
            // Set U
            instruction |= 1 << 24;
            // instr[PC++] = instruction;
            fwrite(&instruction, sizeof(int), 1, outputFile);
            return;
        }
        // numTokens = 3, so there is an immediate value or register provided
        char *p = strchr(tokens[2], ']');
        char *q = strchr(tokens[2], '!');
        if (p != NULL)
        {
            *p = '\0'; // remove ] or ]!
            if (q == NULL)
            {
                if (strchr(tokens[2], '#') != NULL)
                {
                    // Unsigned Immediate Offset
                    // Set U
                    instruction |= 1 << 24;
                    // Set offset
                    int imm12 = getImmediate(tokens[2]) / (mode ? 8 : 4);
                    instruction |= imm12 << 10;
                }
                else
                {
                    // Register Offset
                    // Set 1s
                    instruction |= (1 << 21) + (1 << 14) + (1 << 13) + (1 << 11);
                    // Set xm
                    instruction |= getRegister(tokens[2]) << 16;
                }
                // instr[PC++] = instruction;
                fwrite(&instruction, sizeof(int), 1, outputFile);
                return;
            }
            // Deal with Pre-Indexed after if, so pass through
        }
        // Pre-Indexed & Post-Indexed
        // Set 1s
        instruction |= 1 << 10;
        // Set I: 1 - pre, 0 - posr
        instruction |= (p != NULL) << 11;
        // Set simm9
        int simm9 = getImmediate(tokens[2]);
        // Apply mask because the value is signed
        instruction |= simm9 << 12 & maskBetweenBits(20, 12);
    }
    else
    {
        // Load Literal
        int offset = (getLiteral(tokens[1]) - PC * 4) >> 2;
        instruction |= (offset << 5) & maskBetweenBits(23, 5);
    }
    // instr[PC++] = instruction;
    fwrite(&instruction, sizeof(int), 1, outputFile);
}


// 2.3.3 Branching Instructions
void disassembleB(char *instrname, char *token, FILE *outputFile, int PC)
{
    // Start with xx0101xxxx...
    int instruction = (1 << 28) + (1 << 26);

    // Unconditional
    if (!strcmp(instrname, "b"))
    {
        // Set simm26
        int offset = (getLiteral(token) - PC * 4) >> 2;
        instruction |= offset & maskBetweenBits(25, 0);
    }
    // Register
    else if (!strcmp(instrname, "br"))
    {
        // Set 1s in the right places
        instruction |= (1 << 31) + (1 << 30) + (1 << 25) + maskBetweenBits(20, 16);
        // Set xn
        instruction |= getRegister(token) << 5;
    }
    // Conditional
    else
    {
        // Set 1s in the right places
        instruction |= 1 << 30;
        // Set simm19
        int offset = (getLiteral(token) - PC * 4) >> 2;
        instruction |= (offset << 5) & maskBetweenBits(23, 5);
        // Take by cases regarding the mnemonic
        char *p = instrname + 2;
        switch (*p)
        {
        case 'e': // eq - 0000
            break;
        case 'n': // ne - 0001
            instruction |= 1;
            break;
        case 'g': // ge - 1010 and gt - 1100
            instruction |= (*(p + 1) == 'e')
                               ? 10
                               : 12;
            break;
        case 'l': // lt - 1011 and le - 1101
            instruction |= (*(p + 1) == 't')
                               ? 11
                               : 13;
            break;
        case 'a': // al - 1110
            instruction |= 14;
            break;
        default:
            // Can't reach here
            break;
        }
    }
    // instr[PC++] = instruction;
    fwrite(&instruction, sizeof(int), 1, outputFile);
}


// 2.3.4 Special Instructions/Directives
void disassembleDir(char *dir, FILE *outputFile)
{
    int instruction = getInt(dir);
    fwrite(&instruction, sizeof(int), 1, outputFile);
}