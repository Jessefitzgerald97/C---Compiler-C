#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "tac.h"
#include "codegen.h"

typedef struct ASTnode 
{
    NodeType ntype;
    union
    {
        struct
        {
            char* name;
            int nargs;
            char** argnames;
            void* body;
        } func_defn;

        struct
        {
            char* callee;
            void* args;
        } func_call;

        struct
        {
            void* expr;
            void* then;
            void* els;
        } if_stmt;

        struct
        {
            void* expr;
            void* body;
        } while_stmt;

        struct
        {
            char* lhs;
            void* rhs;
        } assg;

        struct
        {
            void* expr;
        } ret;

        struct
        {
            void* head;
            void* rest;
        } list;

        struct
        {
            char* name;
        } identifier;

        struct
        {
            int val;
        } intconst;

        struct {
            void* op1;
            void* op2;
        } binary;
    } data;
} ASTnode;

TACList* generate_tac();
void test_tac();

void* make_func_defn(char* name, int nargs, char** argnames, void* body)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = FUNC_DEF;
    node->data.func_defn.name = strdup(name);
    node->data.func_defn.nargs = nargs;
    node->data.func_defn.body = body;
    node->data.func_defn.argnames = NULL;

    if (nargs > 0)
    {
        node->data.func_defn.argnames = malloc(nargs * sizeof(char*));
        if (!node->data.func_defn.argnames)
        {
            perror("ERROR: Memory allocation failed\n");
            exit(1);
        }

        for (int i = 0; i < nargs; i++)
        {
            node->data.func_defn.argnames[i] = strdup(argnames[i]);
        }
    }

    return node;
}

void* make_func_call(char* callee, void* args)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = FUNC_CALL;
    node->data.func_call.callee = strdup(callee);
    node->data.func_call.args = args;

    return node;
}

void* make_if_stmt(void* expr, void* then, void* els)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = IF;
    node->data.if_stmt.expr = expr;
    node->data.if_stmt.then = then;
    node->data.if_stmt.els = els;

    return node;
}

void* make_while_stmt(void* expr, void* body)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = WHILE;
    node->data.while_stmt.expr = expr;
    node->data.while_stmt.body = body;

    return node;
}

void* make_assg_stmt(char* lhs, void* rhs)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = ASSG;
    node->data.assg.lhs = strdup(lhs);
    node->data.assg.rhs = rhs;

    return node;
}

void* make_return_stmt(void* expr)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = RETURN;
    node->data.ret.expr = expr;

    return node;
}

void* make_stmt_list(void* head, void* rest)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = STMT_LIST;
    node->data.list.head = head;
    node->data.list.rest = rest;

    return node;
}

void* make_expr_list(void* head, void* rest)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = EXPR_LIST;
    node->data.list.head = head;
    node->data.list.rest = rest;

    return node;
}

void* make_identifier(char* name)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = IDENTIFIER;
    node->data.identifier.name = strdup(name);
    
    return node;
}

void* make_intconst(int val)
{
    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = INTCONST;
    node->data.intconst.val = val;

    return node;
}

void* make_binary_expr(NodeType op, void* op1, void* op2)
{
    if (!op1 || !op2)
    {
        fprintf(stderr, "ERROR: Invalid operators\n");
        exit(1);
    }

    if (op != EQ && op != NE && op != LE && op != LT && op != GE && op != GT && op != ADD 
        && op != SUB && op != MUL && op != DIV && op != OR && op != AND) 
    {
        fprintf(stderr, "ERROR: invalid token\n");
        exit(1);
    }

    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = op;
    node->data.binary.op1 = op1;
    node->data.binary.op2 = op2;

    return node;
}

void* make_unary_expr(NodeType op, void* operand)
{
    if (!operand)
    {
        fprintf(stderr, "ERROR: Invalid operator\n");
        exit(1);
    }

    if (op != UMINUS)
    {
        fprintf(stderr, "ERROR: invalid token\n");
        exit(1);
    }

    ASTnode* node = calloc(1, sizeof(ASTnode));
    if (!node)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    node->ntype = op;
    node->data.binary.op1 = operand;
    node->data.binary.op2 = NULL;

    return node;
}

NodeType ast_node_type(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    return ast->ntype;
}

char* func_def_name(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != FUNC_DEF)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.func_defn.name;
}

int func_def_nargs(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != FUNC_DEF)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.func_defn.nargs;
}

void* func_def_body(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != FUNC_DEF)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.func_defn.body;
}

char* func_def_argname(void* ptr, int n)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != FUNC_DEF)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    if (n < 1 || n > ast->data.func_defn.nargs)
    {
        fprintf(stderr, "ERROR: Invalid argument\n");
        exit(1);
    }

    return ast->data.func_defn.argnames[n - 1];
}

char* func_call_callee(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != FUNC_CALL)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.func_call.callee;
}

void* func_call_args(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != FUNC_CALL)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.func_call.args;
}

void* stmt_list_head(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != STMT_LIST)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.list.head;
}

void* stmt_list_rest(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != STMT_LIST)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.list.rest;
}

void* expr_list_head(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != EXPR_LIST)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.list.head;
}

void* expr_list_rest(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != EXPR_LIST)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.list.rest;
}

char* expr_id_name(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != IDENTIFIER)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.identifier.name;
}

int expr_intconst_val(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != INTCONST)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.intconst.val;
}

void* expr_operand_1(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != EQ && ast->ntype != NE && ast->ntype != LE && ast->ntype != LT && ast->ntype != GE && ast->ntype != GT 
        && ast->ntype != ADD && ast->ntype != SUB && ast->ntype != MUL && ast->ntype != DIV && ast->ntype != AND && ast->ntype != OR && ast->ntype != UMINUS)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.binary.op1;
}

void* expr_operand_2(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != EQ && ast->ntype != NE && ast->ntype != LE && ast->ntype != LT && ast->ntype != GE && ast->ntype != GT 
        && ast->ntype != ADD && ast->ntype != SUB && ast->ntype != MUL && ast->ntype != DIV && ast->ntype != AND && ast->ntype != OR && ast->ntype != UMINUS)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.binary.op2;
}

void* stmt_if_expr(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != IF)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.if_stmt.expr;
}

void* stmt_if_then(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != IF)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.if_stmt.then;
}

void* stmt_if_else(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != IF)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.if_stmt.els;
}

char* stmt_assg_lhs(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != ASSG)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.assg.lhs;
}

void* stmt_assg_rhs(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != ASSG)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.assg.rhs;
}

void* stmt_while_expr(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != WHILE)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.while_stmt.expr;
}

void* stmt_while_body(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != WHILE)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.while_stmt.body;
}

void* stmt_return_expr(void* ptr)
{
    ASTnode* ast = (ASTnode*)ptr;
    if (!ast)
    {
        perror("ERROR: Memory allocation failed\n");
        exit(1);
    }

    if (ast->ntype != RETURN)
    {
        fprintf(stderr, "ERROR: Invalid AST\n");
        exit(1);
    }

    return ast->data.ret.expr;
}
