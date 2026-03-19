/*
* File: tac.c
* Author: Jack Fitzgerald
* Purpose: Traverses a tree of AST nodes to produce three-address instructions
* in the format outlined by cs453
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tac.h"
#include "ast.h"

static int temp_counter = 0;
static int label_counter = 0;

/* Creates a new temporary */
char* new_temp()
{
    char buf[32];
    sprintf(buf, "_t%d", temp_counter);
    temp_counter++;
    char* s = malloc(strlen(buf) + 1);
    if (!s)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    strcpy(s, buf);
    return s;
}

/* Creates a new label */
char* new_label()
{
    char buf[32];
    sprintf(buf, "_L%d", label_counter);
    label_counter++;
    char* s = malloc(strlen(buf) + 1);
    if (!s)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    strcpy(s, buf);
    return s;
}

/* Creates a new TAC struct with the variables populated from the arguments */
TAC* new_tac(TACKind kind, const char* res, const char* arg1, const char* arg2, 
    const char* op, const char* label, int nargs)
    {
        TAC* t = malloc(sizeof(TAC));
        if (!t)
        {
            perror("Memory allocation failed");
            exit(1);
        }
        t->kind = kind;

        if (res)
        {
            t->res = malloc(strlen(res) + 1);
            strcpy(t->res, res);
        }
        else
        {
            t->res = NULL;
        }

        if (arg1)
        {
            t->arg1 = malloc(strlen(arg1) + 1);
            strcpy(t->arg1, arg1);
        }
        else
        {
            t->arg1 = NULL;
        }

        if (arg2)
        {
            t->arg2 = malloc(strlen(arg2) + 1);
            strcpy(t->arg2, arg2);
        }
        else
        {
            t->arg2 = NULL;
        }

        if (op)
        {
            t->op = malloc(strlen(op) + 1);
            strcpy(t->op, op);
        }
        else
        {
            t->op = NULL;
        }

        if (label)
        {
            t->label = malloc(strlen(label) + 1);
            strcpy(t->label, label);
        }
        else
        {
            t->label = NULL;
        }

        t->nargs = nargs;
        t->next = NULL;
        return t;
    }

/* Creates an empty list for TAC structs */
TACList* new_tac_list(void)
{
    TACList* list = malloc(sizeof(TACList));
    if (!list)
    {
        perror("Memory allocation failed");
        exit(1);
    }

    list->head = NULL;
    list->tail = NULL;
    return list;
}

/* Adds a new TAC struct to the end of the TACList */
void tac_append(TACList* list, TAC* inst)
{
    if (!list || !inst)
    {
        return;
    }
    if (list->head == NULL)
    {
        list->head = inst;
        list->tail = inst;
    }
    else
    {
        list->tail->next = inst;
        list->tail = inst;
    }
}

/* Combines two TACLists together */
void tac_join(TACList* a, TACList* b)
{
    if (!a)
    {
        return;
    }
    if (!b || !b->head)
    {
        return;
    }

    if (!a->head)
    {
        a->head = b->head;
        a->tail = b->tail;
    }
    else
    {
        a->tail->next = b->head;
        a->tail = b->tail;
    }
}

/* Frees all variables of a TAC struct */
void free_inst(TAC* t)
{
    if (!t)
    {
        return;
    }

    if (t->res)
    {
        free(t->res);
    }
    if (t->arg1)
    {
        free(t->arg1);
    }
    if (t->arg2)
    {
        free(t->arg2);
    }
    if (t->op)
    {
        free(t->op);
    }
    if (t->label)
    {
        free(t->label);
    }

    free(t);
}

/* Frees the entire TACList of TAC structs */
void free_tac_list(TACList* list)
{
    if (!list)
    {
        return;
    }

    TAC* cur = list->head;

    while (cur != NULL)
    {
        TAC* next = cur->next;
        free_inst(cur);
        cur = next;
    }

    list->head = NULL;
    list->tail = NULL;
}
/* USED FOR DEBUGGING
   Prints a three-address instruction */
static void print_inst(const TAC* t)
{
    if (!t) 
    {
        return;
    }

    switch (t->kind)
    {
        case TAC_LABEL:
            printf("label %s\n", t->label);
            break;

        case TAC_GOTO:
            printf("goto %s\n", t->label);
            break;

        case TAC_IFGOTO:
            printf("if %s %s %s goto %s\n", t->arg1, t->op, t->arg2, t->label);
            break;

        case TAC_ASSIGN:
            printf("%s := %s\n", t->res, t->arg1);
            break;

        case TAC_BINOP:
            printf("%s := %s %s %s\n", t->res, t->arg1, t->op, t->arg2);
            break;

        case TAC_UNARY:
            printf("%s := %s %s\n", t->res, t->op, t->arg1);
            break;

        case TAC_PARAM:
            printf("param %s\n", t->arg1);
            break;

        case TAC_CALL:
            printf("call %s, %d\n", t->arg1 ? t->arg1 : "(null)", t->nargs);
            break;

        case TAC_SETRET:
            printf("set_retval %s\n", t->arg1);
            break;

        case TAC_GETRET:
            printf("get_retval %s\n", t->arg1);
            break;

        case TAC_ENTER:
            printf("enter %s\n", t->arg1);
            break;

        case TAC_LEAVE:
            printf("leave %s\n", t->arg1);
            break;

        case TAC_RETURN:
            printf("return\n");
            break;

        default:
            printf("; UNKNOWN TAC KIND\n");
            break;
    }
}

/* USED FOR DEBUGGING
   prints all three-address instructions from the TACList */
void tac_print(const TACList* list)
{
    if (!list)
    {
        return;
    }

    TAC* cur = list->head;
    while (cur)
    {
        print_inst(cur);
        cur = cur->next;
    }
}

/* Converts an int into a string */
char* int_to_str(int v)
{
    char buf[32];
    sprintf(buf, "%d", v);
    char* s = malloc(strlen(buf) + 1);
    if (!s)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    strcpy(s, buf);
    return s;
}

/* Generates three-address instruction for an expression */
TACList* gen_expr(void* expr, char** result)
{
    if (!expr)
    {
        if (result)
        {
            *result = NULL;
        }
        return new_tac_list();
    }

    NodeType ntype = ast_node_type(expr);
    TACList* list = new_tac_list();

    if (ntype == INTCONST)
    {
        char* val = int_to_str(expr_intconst_val(expr));
        if (result)
        {
            *result = val;
        }
    }

    else if (ntype == IDENTIFIER)
    {
        char* tmp = new_temp();
        char* name = expr_id_name(expr);
        TAC* t = new_tac(TAC_ASSIGN, tmp, name, NULL, NULL, NULL, 0);
        tac_append(list, t);
        *result = tmp;
    }

    else if (ntype == ADD || ntype == SUB || ntype == MUL || ntype == DIV)
    {
        char* t1 = NULL;
        char* t2 = NULL;
        TACList* l1 = gen_expr(expr_operand_1(expr), &t1);
        TACList* l2 = gen_expr(expr_operand_2(expr), &t2);
        tac_join(list, l1);
        tac_join(list, l2);

        const char* op;
        if (ntype == ADD)
        {
            op = "+";
        }
        else if (ntype == SUB)
        {
            op = "-";
        }
        else if (ntype == MUL)
        {
            op = "*";
        }
        else
        {
            op = "/";
        }

        char* tmp = new_temp();
        TAC* t = new_tac(TAC_BINOP, tmp, t1, t2, op, NULL, 0);
        tac_append(list, t);
        *result = tmp;
    }

    else if (ntype == AND || ntype == OR)
    {
        char* left_tmp = NULL;
        char* right_tmp = NULL;
        TACList* l1 = gen_expr(expr_operand_1(expr), &left_tmp);
        TACList* l2 = gen_expr(expr_operand_2(expr), &right_tmp);
        tac_join(list, l1);

        char* label_true = new_label();
        char* label_false = new_label();
        char* label_end = new_label();

        char* result_tmp = new_temp();

        if (ntype == AND)
        {
            TAC* if_left = new_tac(TAC_IFGOTO, NULL, left_tmp, "0", "==", label_false, 0);
            tac_append(list, if_left);

            tac_join(list, l2);

            TAC* if_right = new_tac(TAC_IFGOTO, NULL, right_tmp, "0", "==", label_false, 0);
            tac_append(list, if_right);

            TAC* ttrue = new_tac(TAC_ASSIGN, result_tmp, "1", NULL, NULL, NULL, 0);
            tac_append(list, ttrue);

            TAC* gend = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, label_end, 0);
            tac_append(list, gend);

            TAC* labf = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, label_false, 0);
            tac_append(list, labf);

            TAC* tfalse = new_tac(TAC_ASSIGN, result_tmp, "0", NULL, NULL, NULL, 0);
            tac_append(list, tfalse);

            TAC* labend = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, label_end, 0);
            tac_append(list, labend);
        }
        
        else
        {
            TAC* if_left = new_tac(TAC_IFGOTO, NULL, left_tmp, "0", "!=", label_true, 0);
            tac_append(list, if_left);

            tac_join(list, l2);

            TAC* if_right = new_tac(TAC_IFGOTO, NULL, right_tmp, "0", "!=", label_true, 0);
            tac_append(list, if_right);

            TAC* tfalse = new_tac(TAC_ASSIGN, result_tmp, "0", NULL, NULL, NULL, 0);
            tac_append(list, tfalse);

            TAC* gend = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, label_end, 0);
            tac_append(list, gend);

            TAC* labt = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, label_true, 0);
            tac_append(list, labt);

            TAC* ttrue = new_tac(TAC_ASSIGN, result_tmp, "1", NULL, NULL, NULL, 0);
            tac_append(list, ttrue);

            TAC* labend = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, label_end, 0);
            tac_append(list, labend);
        }

        *result = result_tmp;
    }

    else if (ntype == UMINUS)
    {
        char* t1 = NULL;
        TACList* l1 = gen_expr(expr_operand_1(expr), &t1);
        tac_join(list, l1);
        char* tmp = new_temp();
        TAC* t = new_tac(TAC_UNARY, tmp, t1, NULL, "-", NULL, 0);
        tac_append(list, t);
        *result = tmp;
    }

    else if (ntype == FUNC_CALL)
    {
        char* callee = func_call_callee(expr);
        void* args = func_call_args(expr);
        int nargs = 0;
        void* cur = args;

        while (cur)
        {
            char* arg_tmp = NULL;
            TACList* a_code = gen_expr(expr_list_head(cur), &arg_tmp);
            tac_join(list, a_code);
            TAC* p = new_tac(TAC_PARAM, NULL, arg_tmp, NULL, NULL, NULL, 0);
            tac_append(list, p);
            nargs++;
            cur = expr_list_rest(cur);
        }

        TAC* callt = new_tac(TAC_CALL, NULL, callee, NULL, NULL, NULL, nargs);
        tac_append(list, callt);
        
        char* ret_tmp = new_temp();
        TAC* gt = new_tac(TAC_GETRET, ret_tmp, NULL, NULL, NULL, NULL, 0);
        tac_append(list, gt);

        *result = ret_tmp;
    }

    else
    {
        *result = NULL;
    }

    return list;
}

/* Generates three-address instructions for binary operations */
TACList* gen_relop(void* expr, char** tleft, char** tright)
{
    *tleft = NULL;
    *tright = NULL;
    TACList* list = new_tac_list();
    if (!expr)
    {
        return list;
    }

    void* left = expr_operand_1(expr);
    void* right = expr_operand_2(expr);
    TACList* l1 = gen_expr(left, tleft);
    TACList* l2 = gen_expr(right, tright);
    tac_join(list, l1);
    tac_join(list, l2);
    return list;
}

/* Generates three-address instructions for statements */
TACList* gen_stmt(void* stmt)
{
    if (!stmt)
    {
        return new_tac_list();
    }

    NodeType ntype = ast_node_type(stmt);
    TACList* list = new_tac_list();

    if (ntype == STMT_LIST)
    {
        void* head = stmt_list_head(stmt);
        void* rest = stmt_list_rest(stmt);
        tac_join(list, gen_stmt(head));
        tac_join(list, gen_stmt(rest));
    }

    else if (ntype == ASSG)
    {
        char* lhs = stmt_assg_lhs(stmt);
        void* rhs = stmt_assg_rhs(stmt);
        char* tmp = NULL;
        TACList* rhs_code = gen_expr(rhs, &tmp);
        tac_join(list, rhs_code);
        TAC* t = new_tac(TAC_ASSIGN, lhs, tmp, NULL, NULL, NULL, 0);
        tac_append(list, t);
    }

    else if (ntype == FUNC_CALL)
    {
        char* callee = func_call_callee(stmt);
        void* args = func_call_args(stmt);

        int nargs = 0;
        void* cur = args;
        while (cur)
        {
            char* arg_tmp = NULL;
            TACList* arg_code = gen_expr(expr_list_head(cur), &arg_tmp);
            tac_join(list, arg_code);
            TAC* p = new_tac(TAC_PARAM, NULL, arg_tmp, NULL, NULL, NULL, 0);
            tac_append(list, p);
            nargs++;
            cur = expr_list_rest(cur);
        }
        
        TAC* c = new_tac(TAC_CALL, NULL, callee, NULL, NULL, NULL, nargs);
        tac_append(list, c);
    }

    else if (ntype == IF)
    {
        char* ltrue = new_label();
        char* lfalse = new_label();
        char* lend = new_label();

        void* cond = stmt_if_expr(stmt);
        NodeType ntype2 = ast_node_type(cond);

        if (ntype2 == EQ || ntype2 == NE || ntype2 == LE || ntype2 == LT || ntype2 == GE || ntype2 == GT)
        {
            char* left_tmp = NULL;
            char* right_tmp = NULL;

            TACList* cond_code = gen_relop(cond, &left_tmp, &right_tmp);
            tac_join(list, cond_code);

            const char* relop;
            if (ntype2 == EQ)
            {
                relop = "==";
            }
            else if (ntype2 == NE)
            {
                relop = "!=";
            }
            else if (ntype2 == LE)
            {
                relop = "<=";
            }
            else if (ntype2 == LT)
            {
                relop = "<";
            }
            else if (ntype2 == GE)
            {
                relop = ">=";
            }
            else
            {
                relop = ">";
            }

            TAC* ift = new_tac(TAC_IFGOTO, NULL, left_tmp, right_tmp, relop, ltrue, 0);
            tac_append(list, ift);

            TAC* g = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lfalse, 0);
            tac_append(list, g);

            TAC* labt = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, ltrue, 0);
            tac_append(list, labt);

            tac_join(list, gen_stmt(stmt_if_then(stmt)));

            TAC* jend = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, jend);

            TAC* labf = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lfalse, 0);
            tac_append(list, labf);

            void* elsepart = stmt_if_else(stmt);
            if (elsepart)
            {
                tac_join(list, gen_stmt(elsepart));
            }

            TAC* labend = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, labend);
        }
        else
        {
            char* cond_tmp = NULL;
            TACList* cond_code = gen_expr(cond, &cond_tmp);
            tac_join(list, cond_code);

            TAC* ift = new_tac(TAC_IFGOTO, NULL, cond_tmp, "0", "!=", ltrue, 0);
            tac_append(list, ift);

            TAC* g = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lfalse, 0);
            tac_append(list, g);

            TAC* labt = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, ltrue, 0);
            tac_append(list, labt);

            tac_join(list, gen_stmt(stmt_if_then(stmt)));

            TAC* jend = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, jend);

            TAC* labf = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lfalse, 0);
            tac_append(list, labf);
        
            void* elsepart = stmt_if_else(stmt);
            if (elsepart) 
            {
                tac_join(list, gen_stmt(elsepart));
            }

            TAC* labend = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, labend);
        }
    }

    else if (ntype == WHILE)
    {
        char* lstart = new_label();
        char* lbody = new_label();
        char* lend = new_label();

        TAC* labstart = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lstart, 0);
        tac_append(list, labstart);

        void* cond = stmt_while_expr(stmt);
        NodeType ntype2 = ast_node_type(cond);

        if (ntype2 == EQ || ntype2 == NE || ntype2 == LE || ntype2 == LT || ntype2 == GE || ntype2 == GT)
        {
            char* left_tmp = NULL;
            char* right_tmp = NULL;
            TACList* cond_code = gen_relop(cond, &left_tmp, &right_tmp);
            tac_join(list, cond_code);

            const char* relop;
            if (ntype2 == EQ)
            {
                relop = "==";
            }
            else if (ntype2 == NE)
            {
                relop = "!=";
            }
            else if (ntype2 == LE)
            {
                relop = "<=";
            }
            else if (ntype2 == LT)
            {
                relop = "<";
            }
            else if (ntype2 == GE)
            {
                relop = ">=";
            }
            else
            {
                relop = ">";
            }

            TAC* ift = new_tac(TAC_IFGOTO, NULL, left_tmp, right_tmp, relop, lbody, 0);
            tac_append(list, ift);

            TAC* gend = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, gend);

            TAC* labbody = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lbody, 0);
            tac_append(list, labbody);

            tac_join(list, gen_stmt(stmt_while_body(stmt)));

            TAC* gj = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lstart, 0);
            tac_append(list, gj);

            TAC* labend = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, labend);
        }
        else
        {
            char* cond_tmp = NULL;
            TACList* cond_code = gen_expr(cond, &cond_tmp);
            tac_join(list, cond_code);

            TAC* ift = new_tac(TAC_IFGOTO, NULL, cond_tmp, "0", "!=", lbody, 0);
            tac_append(list, ift);

            TAC* gend = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, gend);

            TAC* labbody = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lbody, 0);
            tac_append(list, labbody);

            tac_join(list, gen_stmt(stmt_while_body(stmt)));

            TAC* gj = new_tac(TAC_GOTO, NULL, NULL, NULL, NULL, lstart, 0);
            tac_append(list, gj);

            TAC* labend = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, lend, 0);
            tac_append(list, labend);
        }
    }

    else if (ntype == RETURN)
    {
        void* rexpr = stmt_return_expr(stmt);
        if (rexpr)
        {
            char* tmp = NULL;
            TACList* code = gen_expr(rexpr, &tmp);
            tac_join(list, code);
            TAC* s = new_tac(TAC_SETRET, NULL, tmp, NULL, NULL, NULL, 0);
            tac_append(list, s);
        }

        TAC* r = new_tac(TAC_RETURN, NULL, NULL, NULL, NULL, NULL, 0);
        tac_append(list, r);
    }

    return list;
}

/* Generates three-address instructions for functions */
TACList* gen_func(void* func_ast)
{
    if (!func_ast)
    {
        return new_tac_list();
    }

    TACList* list = new_tac_list();
    char* name = func_def_name(func_ast);

    TAC* lab = new_tac(TAC_LABEL, NULL, NULL, NULL, NULL, name, 0);
    tac_append(list, lab);

    TAC* en = new_tac(TAC_ENTER, NULL, name, NULL, NULL, NULL, 0);
    tac_append(list, en);

    void* body = func_def_body(func_ast);
    tac_join(list, gen_stmt(body));

    TAC* lv = new_tac(TAC_LEAVE, NULL, name, NULL, NULL, NULL, 0);
    tac_append(list, lv);

    TAC* r = new_tac(TAC_RETURN, NULL, NULL, NULL, NULL, NULL, 0);
    tac_append(list, r);

    return list;
}

/* Entry point to produce a three-address instruction set by 
   traversing the AST tree and calling the necessary functions */
TACList* gen_program(void* ast_root)
{
    TACList* list = new_tac_list();
    if (!ast_root)
    {
        return list;
    }

    NodeType ntype = ast_node_type(ast_root);

    if (ntype == FUNC_DEF)
    {
        TACList* f = gen_func(ast_root);
        tac_join(list, f);
        return list;
    }

    if (ntype == STMT_LIST)
    {
        void* head = stmt_list_head(ast_root);
        void* rest = stmt_list_rest(ast_root);

        if (head)
        {
            TACList* h = gen_program(head);
            tac_join(list, h);
        }

        if (rest)
        {
            TACList* r = gen_program(rest);
            tac_join(list, r);
        }

        return list;
    }

    return list;
}
    