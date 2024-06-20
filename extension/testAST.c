#include <stdio.h>
#include <stdbool.h>

#include "ast.h"
#include "parser.h"
#include "lexer.h"
#include "../src/io.h"

#define testAssignmentsNum 2
#define testLoopsNum 2
#define testFunNum 2
#define INT_MAX_CHARS 20

extern bool sameAST(Statements *ast1, Statements *ast2);
extern bool sameExpression(Expression *e1, Expression *e2);
extern bool sameStatement(Statement *s1, Statement *s2);

// using non-reentrant lexer
extern int yyparse(void);
extern int yylex_destroy (void);


extern Program *program;
extern int yyerrors;

const char *arithmFiles[testAssignmentsNum] = {"testData/arithm1.py", "testData/arithm2.py"};
const char *loopFiles[testLoopsNum] = {"testData/loop1.py", "testData/loop2.py"};
const char *funFiles[testFunNum] = {"testData/fun1.py", "testData/fun2.py"};

const char r = 'r';

bool sameProgram(Program *p1, Program *p2) {
    if (p1 == NULL && p2 == NULL) {
        return true;
    }
    if (p1 == NULL || p2 == NULL) {
        return false;
    }
    return sameAST(p1 -> statements, p2 -> statements);
}

bool sameAST(Statements *ast1, Statements *ast2) {
    if (ast1 == NULL && ast2 == NULL) {
        return true;
    }
    if (ast1 == NULL || ast2 == NULL) {
        return false;
    }
    return sameStatement(ast1->statement, ast2->statement) && sameAST(ast1->next, ast2->next);
}


bool sameExpression(Expression *e1, Expression *e2) {
    if (e1 == NULL && e2 == NULL) {
        return true;
    }
    if (e1 == NULL || e2 == NULL) {
        return false;
    }
    if (e1->tag != e2->tag) {
        return false;
    }

    switch (e1->tag) {
        case EXPR_NAME:
            return strcmp(e1->name->name, e2->name->name) == 0;

        case EXPR_INT:
            return e1->int_value->value == e2->int_value->value;

        case EXPR_BINARY_OP:
            return strcmp(e1->binary_op->op, e2->binary_op->op) == 0 &&
                   sameExpression(e1->binary_op->left, e2->binary_op->left) &&
                   sameExpression(e1->binary_op->right, e2->binary_op->right);

        case EXPR_UNARY_OP:
            return strcmp(e1->unary_op->op, e2->unary_op->op) == 0 &&
                   sameExpression(e1->unary_op->expression, e2->unary_op->expression);

        case EXPR_FUNCTION_CALL:
            if (strcmp(e1->function_call->name, e2->function_call->name) != 0 ||
                e1->function_call->arg_count != e2->function_call->arg_count) {
                return false;
            }

            Arguments *a1 = e1->function_call->args;
            Arguments *a2 = e2->function_call->args;

            while (a1 && a2) {
                if (!sameExpression(a1->arg, a2->arg)) {
                    return false;
                }
                a1 = a1->next;
                a2 = a2->next;
            }
            return a1 == NULL && a2 == NULL;

        default:
            return false;
    }
}

bool sameStatement(Statement *s1, Statement *s2) {
    if (s1->tag != s2->tag) {
        return false;
    }

    switch (s1->tag) {
        case ASSIGNMENT_STMT:
            return strcmp(s1->assignment_stmt->name, s2->assignment_stmt->name) == 0 &&
                   sameExpression(s1->assignment_stmt->expression, s2->assignment_stmt->expression);

        case FLOW_STMT:
            return strcmp(s1->flow_stmt->name, s2->flow_stmt->name) == 0 &&
                   sameExpression(s1->flow_stmt->expression, s2->flow_stmt->expression);

        case IF_STMT:
            return sameExpression(s1->if_stmt->condition, s2->if_stmt->condition) &&
                   sameAST(s1->if_stmt->then_block, s2->if_stmt->then_block) &&
                   sameAST(s1->if_stmt->else_block, s2->if_stmt->else_block);

        case WHILE_STMT:
            return sameExpression(s1->while_stmt->condition, s2->while_stmt->condition) &&
                   sameAST(s1->while_stmt->block, s2->while_stmt->block);

        case FOR_STMT:
            return strcmp(s1->for_stmt->var, s2->for_stmt->var) == 0 &&
                   sameExpression(s1->for_stmt->range, s2->for_stmt->range) &&
                   sameAST(s1->for_stmt->block, s2->for_stmt->block);

        case FUNCTION_DEF:
            if (strcmp(s1->function_def->name, s2->function_def->name) != 0 ||
                s1->function_def->param_count != s2->function_def->param_count ||
                !sameAST(s1->function_def->body, s2->function_def->body)) {
                return false;
            }

            Parameters *p1 = s1->function_def->parameters;
            Parameters *p2 = s2->function_def->parameters;

            while (p1 && p2) {
                if (strcmp(p1->parameter->name, p2->parameter->name) != 0) {
                    return false;
                }
                p1 = p1->next;
                p2 = p2->next;
            }
            return p1 == NULL && p2 == NULL;

        default:
            return false;
    }
}

static char *get_Expression_str(Expression *e) {
    char *str;
    switch(e -> tag) {
        case EXPR_NAME:
            // e -> name is a pointer to Name, which is a struct containing char *name
            str = malloc((strlen(e -> name -> name) + 1) * sizeof(char));
            if (str == NULL) {
                exit(EXIT_FAILURE);
            }
            strcpy(str, e -> name -> name);
            break;
        case EXPR_INT: {
            str = malloc(INT_MAX_CHARS * sizeof(char));
            if (str == NULL) {
                exit(EXIT_FAILURE);
            }
            snprintf(str, INT_MAX_CHARS * sizeof(char), "%" PRId64, e -> int_value -> value);
            break;
        }
        case EXPR_BINARY_OP: {
            BinaryOp *b = e -> binary_op;
            char *s1 = get_Expression_str((b -> left));
            char *s2 = get_Expression_str((b -> right));
            str = malloc(strlen(s1) + strlen(s2) + strlen(b -> op) + 5);
            
            // combining strings
            char *p = str;

            *p = '(';
            p++;
            strcpy(p, s1);
            p += strlen(s1);
            *p = ')';
            p++;

            strcpy(p, b -> op);
            p += strlen(b -> op);

            *p = '(';
            p++;
            strcpy(p, s2);
            p += strlen(s2);
            *p = ')';
            p++;
            *p = '\0';
            // freeing
            free(s1);
            free(s2);
            break;
        }
        case EXPR_UNARY_OP: {
            UnaryOp *u1 = e -> unary_op;
            char *s1 = get_Expression_str((u1 -> expression));
            str = malloc(strlen(s1) + strlen(u1 -> op) + 3);
            
            // combining
            char *p = str;
            strcpy(p, u1 -> op);
            p += strlen(u1 -> op);
            *p++ = '(';
            strcpy(p, s1);
            *p++ = ')';
            *p = '\n';
    
            // freeing
            free(s1);
            break;
        }
        case EXPR_FUNCTION_CALL: {
            FunctionCall *fc = e -> function_call;
            int arg_num = fc -> arg_count;
            char **arg_exprs = malloc(arg_num * sizeof(char *));
            int params_size = 0;

            char *arg_str;
            int i = 0;
            for (Arguments *args = fc -> args ; args != NULL ; args = args -> next) {
                arg_str = get_Expression_str((args -> arg));
                arg_exprs[i] = arg_str;
                params_size += strlen(arg_str);
                i++;
            }
            str = malloc((strlen(fc -> name) + 2 + params_size + 2 * ((arg_num >= 1) ? arg_num - 1 : 0)) * sizeof(char) + 1);
            
            // combining (with freeing)
            char *p = str;
            strcpy(p, fc -> name);
            p += strlen(fc -> name);
            *p = '(';
            p += 1;

            for (int i = 0 ; i < arg_num ; i++) {
                strcpy(p, arg_exprs[i]);
                p += strlen(arg_exprs[i]);
                strcpy(p, ", ");

                free(arg_exprs[i]);
                p += 2;
            }
            p -= 2; // backtrack to get rid of last ", "
            *p = ')';
            free(arg_exprs);
            break;
         }
    }
    return str;
}



void print_ast(Statements *statements) {
    if (statements == NULL) {
        return;
    }
    switch(statements -> statement -> tag) {
        case ASSIGNMENT_STMT: {
            AssignmentStmt *assignment_stmt = statements -> statement -> assignment_stmt;
            printf("%s = ", assignment_stmt -> name);
            printf("%s\n", get_Expression_str(assignment_stmt -> expression));
            break;
        }
        case FLOW_STMT: {
            FlowStmt *flow_stmt = statements -> statement -> flow_stmt;
            printf("%s ", flow_stmt -> name);
            printf("%s", get_Expression_str(flow_stmt -> expression));
            printf("\n");
            break;
        }
        case IF_STMT: {
            IfStmt *if_stmt = statements -> statement -> if_stmt;
            printf("if %s:\n", get_Expression_str(if_stmt -> condition));
            printf("  ");
            print_ast(if_stmt -> then_block);
            printf("else \n");
            print_ast(if_stmt -> else_block);
            break;
        }
            
        case WHILE_STMT: {
            WhileStmt *while_stmt = statements -> statement -> while_stmt;
            printf("while %s\n", get_Expression_str(while_stmt -> condition));
            print_ast(while_stmt -> block);
            break;
        }
        
        case FOR_STMT: {
            ForStmt *for_stmt = statements -> statement -> for_stmt;
            printf("for %s in range(%s)\n", for_stmt -> var, get_Expression_str(for_stmt -> range));
            printf("  ");
            print_ast(for_stmt -> block);
            break;
        }
        case FUNCTION_DEF: {
            FunctionDef *function_def = statements -> statement -> function_def;
            printf("%s(", function_def -> name);
            for (Parameters *current = function_def -> parameters ; current != NULL ; current = current -> next) {
                if (current -> next != NULL) {
                    printf("%s, ", current -> parameter -> name);
                }
                // is last parameter
                else {
                    printf("%s)\n", current -> parameter -> name);

                }
                
            }
        }
        default: {
            printf("hi");
            break;
        }
    }
    print_ast(statements -> next);
}

void print_program(Program *p) {
    if (p != NULL) {
        print_ast(p -> statements);
    }
}

void testAssignments() {
    // Creating expected results
    Program *expectedPrograms[testAssignmentsNum];

    // arithm1.py
    // a = 2
    Int *arithm1_int_2 = create_int(2);
    Expression *arithm1_expr_2 = create_expression(EXPR_INT, arithm1_int_2);
    AssignmentStmt *arithm1_assign_a_2 = create_assignment_stmt("a", arithm1_expr_2);
    Statement *arithm1_stmt1 = create_statement(ASSIGNMENT_STMT, arithm1_assign_a_2);

    // b = 3
    Int *arithm1_int_3 = create_int(3);
    Expression *arithm1_expr_3 = create_expression(EXPR_INT, arithm1_int_3);
    AssignmentStmt *arithm1_assign_b_3 = create_assignment_stmt("b", arithm1_expr_3);
    Statement *arithm1_stmt2 = create_statement(ASSIGNMENT_STMT, arithm1_assign_b_3);

    // a = a + b
    Name *arithm1_name_a2 = create_name("a");
    Name *arithm1_name_b = create_name("b");
    Expression *arithm1_expr_a = create_expression(EXPR_NAME, arithm1_name_a2);
    Expression *arithm1_expr_b = create_expression(EXPR_NAME, arithm1_name_b);
    BinaryOp *arithm1_binary_op = create_binary_op("+", arithm1_expr_a, arithm1_expr_b);
    Expression *arithm1_expr_a_plus_b = create_expression(EXPR_BINARY_OP, arithm1_binary_op);
    AssignmentStmt *arithm1_assign_a_a_plus_b = create_assignment_stmt("a", arithm1_expr_a_plus_b);
    Statement *arithm1_stmt3 = create_statement(ASSIGNMENT_STMT, arithm1_assign_a_a_plus_b);

    // Link statements
    Statements *arithm1_statements3 = create_statements(arithm1_stmt3, NULL);
    Statements *arithm1_statements2 = create_statements(arithm1_stmt2, arithm1_statements3);
    Statements *arithm1_statements1 = create_statements(arithm1_stmt1, arithm1_statements2);

    // Create program
    expectedPrograms[0] = create_program(arithm1_statements1);

    // arithm2.py

    // a = 3 * 4
    Int *arithm2_int_3 = create_int(3);
    Int *arithm2_int_4 = create_int(4);
    Expression *arithm2_expr_3 = create_expression(EXPR_INT, arithm2_int_3);
    Expression *arithm2_expr_4 = create_expression(EXPR_INT, arithm2_int_4);
    BinaryOp *arithm2_binary_op_mult = create_binary_op("*", arithm2_expr_3, arithm2_expr_4);
    Expression *arithm2_expr_3_mult_4 = create_expression(EXPR_BINARY_OP, arithm2_binary_op_mult);
    AssignmentStmt *arithm2_assign_a_3_mult_4 = create_assignment_stmt("a", arithm2_expr_3_mult_4);
    Statement *arithm2_stmt1 = create_statement(ASSIGNMENT_STMT, arithm2_assign_a_3_mult_4);

    // b = a - 3
    Name *arithm2_name_a = create_name("a");
    Expression *arithm2_expr_a = create_expression(EXPR_NAME, arithm2_name_a);
    Expression *arithm2_expr_int_3 = create_expression(EXPR_INT, arithm2_int_3);
    BinaryOp *arithm2_binary_op_sub = create_binary_op("-", arithm2_expr_a, arithm2_expr_int_3);
    Expression *arithm2_expr_a_sub_3 = create_expression(EXPR_BINARY_OP, arithm2_binary_op_sub);
    AssignmentStmt *arithm2_assign_b_a_sub_3 = create_assignment_stmt("b", arithm2_expr_a_sub_3);
    Statement *arithm2_stmt2 = create_statement(ASSIGNMENT_STMT, arithm2_assign_b_a_sub_3);

    // Link statements
    Statements *arithm2_statements2 = create_statements(arithm2_stmt2, NULL);
    Statements *arithm2_statements1 = create_statements(arithm2_stmt1, arithm2_statements2);

    // Create program
    expectedPrograms[1] = create_program(arithm2_statements1);

    // comparing
    for (int i = 0 ; i < testAssignmentsNum ; i++) {
        yyin = loadInputFile(arithmFiles[i], NULL, &r);
        if (yyparse() == 0) {
            if (sameProgram(expectedPrograms[i], program)) {
                printf("TEST %d OK\n", i);
            }
            else {
                printf("FAIL\n");
                printf("  EXPECTED\n");
                print_program(expectedPrograms[i]);
                printf("    GOT\n");
                print_program(program);
            }
        } 
        else {
            fprintf(stderr, "Parsing failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

void testLoops() {
    // Creating expected results
    Program *expectedPrograms[testLoopsNum];


    // loop1.py
    // for i in range(3):
    //     count = count + 1

    // Create range expression 3
    Int *loop1_int_3 = create_int(3);
    Expression *loop1_expr_3 = create_expression(EXPR_INT, loop1_int_3);

    // Create increment count expression
    Name *loop1_name_count2 = create_name("count");
    Expression *loop1_expr_count2 = create_expression(EXPR_NAME, loop1_name_count2);
    Int *loop1_int_1 = create_int(1);
    Expression *loop1_expr_1 = create_expression(EXPR_INT, loop1_int_1);
    BinaryOp *loop1_binary_op_plus = create_binary_op("+", loop1_expr_count2, loop1_expr_1);
    Expression *loop1_expr_count_plus_1 = create_expression(EXPR_BINARY_OP, loop1_binary_op_plus);
    AssignmentStmt *loop1_assign_count = create_assignment_stmt("count", loop1_expr_count_plus_1);
    Statement *loop1_stmt_body = create_statement(ASSIGNMENT_STMT, loop1_assign_count);

    // Create for loop statement
    Statements *loop1_body = create_statements(loop1_stmt_body, NULL);
    ForStmt *loop1_for_stmt = create_for_stmt("i", loop1_expr_3, loop1_body);
    Statement *loop1_stmt = create_statement(FOR_STMT, loop1_for_stmt);

    // Link statements
    Statements *loop1_statements = create_statements(loop1_stmt, NULL);

    // Create program
    expectedPrograms[0] = create_program(loop1_statements);

    // printing for debugging
    print_program(expectedPrograms[0]);
    
    // Comparing
    for (int i = 0; i < testLoopsNum; i++) {
        yyin = loadInputFile(loopFiles[i], NULL, &r);
        if (yyparse() == 0) {
            if (sameProgram(expectedPrograms[i], program)) {
                printf("TEST %d OK\n", i);
            } else {
                printf("FAIL\n");
                printf("  EXPECTED\n");
                print_program(expectedPrograms[i]);
                printf("    GOT\n");
                print_program(program);
            }
        } else {
            fprintf(stderr, "Parsing failed\n");
            exit(EXIT_FAILURE);
        }
    }

    // Free the expected programs
    for (int i = 0; i < testLoopsNum; i++) {
        free_program(expectedPrograms[i]);
    }
}



/*
void testFunDefs() {
    exit(EXIT_FAILURE);
}

void testCombined() {
    exit(EXIT_FAILURE);
}
*/


int main(void) {
    // testAssignments();
    testLoops();

    return 0;
}
