/*
* File: codegen.c
* Author: Jack Fitzgerald
* Purpose: Produces MIPS assembly code by translating a three-address
* instruction set and the variable types and offsets provided by the 
* symbol table
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "tac.h"
#include "parser.h"

extern VarInfo* sym_globals;
extern FuncInfo* sym_functions;

static const char* cur_func = NULL;
static int label_count = 0;

/* Formats a label */
static char* mangle(const char* name)
{
    if (!name) 
    {
        return NULL;
    }

    if (strncmp(name, "_L", 2) == 0)
    {
        return strdup(name);
    }

    if (name[0] == '_') 
    {
        return strdup(name);
    }

    size_t n = strlen(name) + 2;
    char* out = malloc(n);
    if (!out) 
    { perror("Memory allocation failed"); 
        exit(1); 
    
    }
    out[0] = '_';
    strcpy(out + 1, name);
    return out;
}

/* Checks if a variable is a temporary */
static int is_temp(const char* name) 
{
    return name && name[0] == '_' && name[1] == 't';
}

/* Returns an available temporary register */
static const char* temp_reg(const char* name) 
{
    if (!is_temp(name)) 
    {
        return NULL;
    }

    switch (name[2]) {
        case '0': return "$t0";
        case '1': return "$t1";
        case '2': return "$t2";
        case '3': return "$t3";
        case '4': return "$t4";
        case '5': return "$t5";
        case '6': return "$t6";
        case '7': return "$t7";
        case '8': return "$t8";
        case '9': return "$t9";
        default: return NULL;
    }
}

/* Creates a new label */
char* new_label_code(void) 
{
    char* buf = malloc(32);
    if (!buf) 
    {   
        perror("Memory allocation failed"); 
        exit(1); 
    }
    sprintf(buf, "_L%d", label_count++);
    return buf;
}

/* Checks if a character is a number character */
int is_number(const char* s) 
{
    if (!s || !*s) 
    {
        return 0;
    }
    if (*s == '-' || *s == '+') 
    {
        s++;
    }
    
    while (*s) 
    { 
        if (*s < '0' || *s > '9') 
        {
            return 0;
        }
        s++; 
    }
    return 1;
}

/* Checks first local variables, then global variables in the
   symbol table to find a specific variable */
static VarInfo* find_var(const char* name)
{
    if (!name)
    {
        return NULL;
    }

    FuncInfo* f = sym_functions;
    while (f)
    {
        if (strcmp(f->name, cur_func) == 0)
        {
            VarInfo* v = f->vars;
            while (v)
            {
                if (strcmp(v->name, name)== 0)
                {
                    return v;
                }
                v = v->next;
            }
        }
        f = f->next;
    }

    VarInfo* g = sym_globals;
    while (g)
    {
        if (strcmp(g->name, name) == 0)
        {
            return g;
        }
        g = g->next;
    }
    
    return NULL;
}

/* Loads a variable into a register */
static void load(const char* name, const char* reg)
{
    if (!name)
    {
        printf("    li %s, 0\n", reg);
        return;
    }

    if (is_temp(name))
    {
        const char* tmpreg = temp_reg(name);
        if (tmpreg && strcmp(reg, tmpreg) != 0)
        {
            printf("    move %s, %s\n", reg, tmpreg);
        }
        return;
    }

    if (is_number(name))
    {
        printf("    li %s, %s\n", reg, name);
        return;
    }

    VarInfo* v = find_var(name);
    if (!v) {
        fprintf(stderr, "ERROR: Variable not found: %s\n", name);
        exit(1);
    }

    if (v->is_param == -1) {
        char* m = mangle(name);
        printf("    lw %s, %s\n", reg, m);
        free(m);
    } else {
        printf("    lw %s, %d($fp)\n", reg, v->offset);
    }
}

/* Saves a variable from a register */
static void store(const char* name, const char* reg)
{
    if (!name)
    {
        fprintf(stderr, "ERROR: Store to NULL\n");
        exit(1);
    }

    if (name[0] == '_' && name[1] == 't')
    {
        int n = name[2] - '0';
        char tmpreg[4];
        sprintf(tmpreg, "$t%d", n);
        if (strcmp(reg, tmpreg) != 0)
        {
            printf("    move %s, %s\n", tmpreg, reg);
        }
        return;
    }

    VarInfo* v = find_var(name);
    if (!v) {
        fprintf(stderr, "ERROR: Variable not found: %s\n", name);
        exit(1);
    }

    if (v->is_param == -1) {
        char* m = mangle(name);
        printf("    sw %s, %s\n", reg, m);
        free(m);
    } else {
        printf("    sw %s, %d($fp)\n", reg, v->offset);
    }
}

/* Prints the necessary header and global variables */
void emit_globals(void)
{
    printf(".data\n");
    printf(".align 2\n");

    VarInfo* g = sym_globals;
    while (g)
    {
        char* m = mangle(g->name);
        printf("%s: .space 4\n", m);
        free(m);
        g = g->next;
    }

    printf(".align 2\n");
    printf("_nl: .asciiz \"\\n\"\n");
}

/* Prints the println function */
void emit_println(void)
{
    printf(".text\n");
    printf("# _println: print out an integer followed by a newline\n");
    printf("_println:\n");
    printf("    li $v0, 1\n");
    printf("    lw $a0, 0($sp)\n");
    printf("    syscall\n");
    printf("    li $v0, 4\n");
    printf("    la $a0, _nl\n");
    printf("    syscall\n");
    printf("    jr $ra\n");
}

/* Computes the size to extend the stack based on the number 
   of parameters in a function */
static int compute_frame_size(FuncInfo* f)
{
    if (!f) 
    {
        return 0;
    }

    int sz = 0;
    VarInfo* v = f->vars;
    while (v)
    {
        if (!v->is_param)
        {
            sz += 4;
        }
        v = v->next;
    }

    return sz;
}

/* Translates a three-address instruction into MIPS assembly code */
static void gen_inst(const TAC* t)
{
    if (!t)
    {
        return;
    }

    switch (t->kind)
    {
        case TAC_ASSIGN:
        {
            if (is_temp(t->res)) 
            {
                load(t->arg1, temp_reg(t->res));
            } 
            else 
            {
            load(t->arg1, "$t0");
            store(t->res, "$t0");
            }
            break;
        }

        case TAC_BINOP:
        {
            printf("# BINOP: res=%s arg1=%s arg2=%s op=%s\n", t->res, t->arg1, t->arg2, t->op);
            load(t->arg1, "$t0");
            load(t->arg2, "$t1");
            
            if (strcmp(t->op, "+") == 0) 
            {
                printf("    add $t2, $t0, $t1\n");
            }
            else if (strcmp(t->op, "-") == 0) 
            {
                printf("    sub $t2, $t0, $t1\n");
            }
            else if (strcmp(t->op, "*") == 0) 
            {
                printf("    mult $t0, $t1\n");
                printf("    mflo $t2\n");
            }
            else if (strcmp(t->op, "/") == 0) 
            {
                printf("    div $t0, $t1\n");
                printf("    mflo $t2\n");
            }

            store(t->res, "$t2");
            break;
        }

        case TAC_UNARY:
        {
            load(t->arg1, "$t0");

            if (strcmp(t->op, "-") == 0)
            {
                printf("    sub $t2, $zero, $t0\n");
            }
            else 
            { 
                fprintf(stderr, "ERROR: Invalid operator\n"); 
                exit(1); 
            }

            store(t->res, "$t2");
            break;
        }

        case TAC_LABEL:
        {
            if (strcmp(t->label, "main") == 0)
            {
                break;
            }

            FuncInfo* f = sym_functions;
            int is_func = 0;
            while (f)
            {
                if (strcmp(f->name, t->label) == 0)
                {
                    is_func = 1;
                    break;
                }
                f = f->next;
            }

            if (is_func)
            {
                break;
            }

            char* lab = mangle(t->label);
            printf("%s:\n", lab);
            free(lab);
            break;
        }

        case TAC_GOTO:
        {
            char* lab = mangle(t->label);
            printf("    j %s\n", lab);
            free(lab);
            break;
        }

        case TAC_IFGOTO:
        {
            load(t->arg1, "$t0");
            load(t->arg2, "$t1");

            char* lab = mangle(t->label);

            if (strcmp(t->op, "==") == 0) 
            {
                printf("    beq $t0, $t1, %s\n", lab);
            }
            else if (strcmp(t->op, "!=") == 0) 
            {
                printf("    bne $t0, $t1, %s\n", lab);
            }
            else if (strcmp(t->op, "<") == 0) 
            {
                printf("    blt $t0, $t1, %s\n", lab);
            }
            else if (strcmp(t->op, "<=") == 0) 
            {
                printf("    ble $t0, $t1, %s\n", lab);
            }
            else if (strcmp(t->op, ">") == 0) 
            {
                printf("    bgt $t0, $t1, %s\n", lab);
            }
            else if (strcmp(t->op, ">=") == 0) 
            {
                printf("    bge $t0, $t1, %s\n", lab);
            }
            else 
            { 
                fprintf(stderr, "ERROR: Invalid operator\n"); 
                exit(1); 
            }

            free(lab);
            break;
        }

        case TAC_PARAM:
        {
            load(t->arg1, "$t0");
            printf("    la $sp, -4($sp)\n");
            printf("    sw $t0, 0($sp)\n");
            break;
        }

        case TAC_CALL:
        {
            char* callee = mangle(t->arg1);
            printf("    jal %s\n", callee);
            free(callee);

            if (t->nargs > 0)
            {
                printf("    la $sp, %d($sp)\n", 4 * t->nargs);
            }

            if (t->res)
            {
                store(t->res, "$v0");
            }
            
            break;
        }

        case TAC_SETRET:
        {
            load(t->arg1, "$v0");
            break;
        }

        case TAC_GETRET:
        {
            store(t->arg1, "$v0");
            break;
        }

        case TAC_ENTER:
        {
            cur_func = t->arg1;

            if (strcmp(cur_func, "main") == 0)
            {
                printf("main:\n");
            }
            else
            {
                char* func_label = mangle(cur_func);
                printf("%s:\n", func_label);
                free(func_label);
            }

            printf("    la $sp, -8($sp)\n");
            printf("    sw $fp, 4($sp)\n");
            printf("    sw $ra, 0($sp)\n");
            printf("    la $fp, 0($sp)\n");

            FuncInfo* f = sym_functions;
            while (f)
            {
                if (strcmp(f->name, cur_func) == 0)
                {
                    break;
                }
                f = f->next;
            }

            if (f)
            {
                int bytes = compute_frame_size(f);
                if (bytes > 0) 
                {
                    printf("    la $sp, -%d($sp)\n", bytes);
                }
            }

            break;
        }

        case TAC_LEAVE:
        {
            FuncInfo* f = sym_functions;
            while (f)
            {
                if (strcmp(f->name, cur_func) == 0)
                {
                    break;
                }
                f = f->next;
            }

            int bytes = 0;
            if (f) 
            {
                bytes = compute_frame_size(f);
            }

            if (bytes > 0) 
            {
                printf("    la $sp, %d($sp)\n", bytes);
            }

            printf("    lw $ra, 0($fp)\n");
            printf("    lw $fp, 4($fp)\n");
            printf("    la $sp, 8($sp)\n");

            cur_func = NULL;
            break;
        }

        case TAC_RETURN:
        {
            printf("    jr $ra\n");
            break;
        }

        default:
        {
            fprintf(stderr, "ERROR: Invalid TAC\n");
            exit(1);
        }
    }
}

/* USED FOR DEBUGGING 
   prints comments containing the global variables and functions 
   in a three-address instruction set */
static void debug_symbols(void)
{
    printf("# --- DEBUG SYMBOLS ---\n");

    printf("# Globals:\n");
    VarInfo* g = sym_globals;
    while (g) {
        printf("#   %s  offset=%d  is_param=%d\n", 
            g->name, g->offset, g->is_param);
        g = g->next;
    }

    printf("# Functions:\n");
    FuncInfo* f = sym_functions;
    while (f) {
        printf("# Function %s  num_params=%d\n", 
            f->name, f->num_params);

        VarInfo* v = f->vars;
        while (v) {
            printf("#     var %s  offset=%d  is_param=%d\n",
                v->name, v->offset, v->is_param);
            v = v->next;
        }
        f = f->next;
    }
}
/* Entry point to iterate through the TACList of three-address 
   instructions and translate them into MIPS assembly code */
void gen_code(const TACList* list) 
{
    debug_symbols();
    emit_globals();
    emit_println();

    printf(".text\n");
    printf(".globl main\n");

    const TAC* cur = list->head;
    while (cur) 
    {
        gen_inst(cur);
        cur = cur->next;
    }
}