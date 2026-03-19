/*
 * File: parser.c
 * Author: Jack Fitzgerald
 * Purpose: Checks if an input token sequence is sytanctically legal with respect
 * to the sytanx rules for the G0 language. Reports an error message at a specific
 * line number a sytanx error is found.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "scanner.h"
#include "ast.h"
#include "tac.h"
#include "codegen.h"

extern int get_token();
extern char* lexeme;
extern int line;
extern int chk_decl_flag;
extern int print_ast_flag;
extern int gen_code_flag;
int bracket_flag = 0;

static int token;

static Identifier* global_vars = NULL;
static Identifier* global_funcs = NULL;
static Identifier* local_vars = NULL;

VarInfo* sym_globals = NULL;
FuncInfo* sym_functions = NULL;

char** make_global_names_array(int* out_count);
void insert_id();
void sym_add_global(const char* name);
int check_duplicate();
int check_declared();
void check_arguments();
void free_list();
void report_error();
void match();
int parse();
void* prog();
void var_decl();
void id_list();
void* func_defn();
void type();
int opt_formals();
int formals();
void opt_var_decls();
void* opt_stmt_list();
void* stmt();
void* if_stmt();
void* while_stmt();
void* return_stmt();
void* assg_stmt();
void* fn_call();
void* opt_expr_list();
int expr_list();
void* bool_exp();
void* arith_exp();
NodeType relop();
void* make_func_defn();
void* make_func_call();
void* make_stmt_list();
void* make_if_stmt();
void* make_while_stmt();
void* make_assg_stmt();
void* make_return_stmt();
void* make_expr_list();
void* make_identifier();
void* make_intconst();
void* make_binary_expr();
void* make_unary_expr();
void* parse_primary();
void* parse_unary();
void* parse_muldiv();
void* parse_addsub();
void* parse_relational();
void* parse_and();
void* parse_or();

/* Checks the flag, checks for duplicates, then inserts a new node
*  into a linked list
*/
void insert_id(Identifier** list, const char* name, int flag, int is_param)
{
    if (flag == 0)
    {
        return;
    }
    
    if (check_duplicate(*list, name) == 1)
    {
        fprintf(stderr, "ERROR: Multiple declarations at LINE: %d\n", line);
        exit(1);
    }
    
    else
    {
        Identifier* new = malloc(sizeof(Identifier));
        if (!new)
        {
            perror("ERROR: Memory allocation failed\n");
            exit(1);
        }
        new->name = strdup(name);
        new->args = 0;
        new->is_param = is_param;
        new->offset = 0;
        new->next = *list;
        *list = new;
    }
}

/* Adds variable so sym_globals */
void sym_add_global(const char* name) 
{
    VarInfo* v = malloc(sizeof(VarInfo));
    if (!v)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    v->name = strdup(name);
    v->is_param = -1;
    v->offset = 0;
    v->next = sym_globals;
    sym_globals = v;
}

/* Adds variable to a function's variables with parameter value */
void sym_add_param(FuncInfo* f, const char* name, int offset) 
{
    VarInfo* v = malloc(sizeof(VarInfo));
    if (!v)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    v->name = strdup(name);
    v->is_param = 1;
    v->offset = offset;
    v->next = f->vars;
    f->vars = v;
}

/* Adds variable to a function's variables without parameter value */
void sym_add_local(FuncInfo* f, const char* name, int offset) 
{
    VarInfo* v = malloc(sizeof(VarInfo));
    if (!v)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    v->name = strdup(name);
    v->is_param = 0;
    v->offset = offset;
    v->next = f->vars;
    f->vars = v;
}

/* Checks for duplicates in a linked list */
int check_duplicate(Identifier* list, const char* name)
{
    Identifier* cur = list;
    while (cur != NULL)
    {
        if (strcmp(cur->name, name) == 0)
        {
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

int check_declared(const char* name)
{
    // check local
    Identifier* cur = local_vars;
    while (cur != NULL)
    {
        if (strcmp(cur->name, name) == 0)
        {
            return 1;
        }
        cur = cur->next;
    }

    // check global
    cur = global_vars;
    while (cur != NULL)
    {
        if (strcmp(cur->name, name) == 0)
        {
            return 1;
        }
        cur = cur->next;
    }

    return 0;
}

/* Verifies the correct number of arguments for a function */
void check_arguments(const char* name, int count)
{
    Identifier* funcs = global_funcs;
    while (funcs != NULL)
    {
        if (strcmp(funcs->name, name) == 0)
        {
            if (funcs->args != count)
            {
                fprintf(stderr, "ERROR: Invalid arguments at line %d\n", line);
                exit(1);
            }
            return;
        }
        funcs = funcs->next;
    }
}

/* Frees the memory of a linked list */
void free_list(Identifier** list)
{
    Identifier* tmp;
    while (*list != NULL)
    {
        tmp = *list;
        *list = (*list)->next;
        free(tmp->name);
        free(tmp);
    }
}

/* Prints syntax error at current line number */
void report_error(const char* error)
{
    fprintf(stderr, "ERROR: %s for %s at LINE %d\n", error, lexeme, line);
    exit(1);
}

/* Checks if current token matches expected token for syntax 
* rules and reports an error if it doesn't
*/
void match(int exp)
{
    if (token == exp)
    {
        token = get_token();
    }
    else
    {
        report_error("Unexpected token");
    }
}

/* Calls prog() and reports syntax error if token doesn't reach EOF */
int parse()
{
    Identifier* ptln = malloc(sizeof(Identifier));
    if (!ptln)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    ptln->name = strdup("println");
    ptln->args = 1;
    ptln->is_param = -1;
    ptln->offset = 0;
    ptln->next = global_funcs;
    global_funcs = ptln;

    FuncInfo* pf = malloc(sizeof(FuncInfo));
    if (!pf) 
    { 
        perror("Memory allocation failed"); exit(1); 
    }
    pf->name = strdup("println");
    pf->num_params = 1;
    pf->vars = NULL;
    pf->next = sym_functions;
    sym_functions = pf;

    token = get_token();
    void* ast = prog();

    if (token != EOF)
    {
        report_error("Syntax error");
    }

    if (gen_code_flag == 1)
    {
        TACList* tac = gen_program(ast);
        gen_code(tac);

        /*
        tac_print(tac);
        free_tac_list(tac);
        return 0;
        */
    }

    return 0;
}

/* Calls func_defn() for as long as the current token matches kwINT */
void* prog()
{
    void* stack[256];
    int count = 0;

    while (token == kwINT)
    {
        type();
        if (token != ID)
        {
            report_error("Unexpected token");
        }
        char* name = strdup(lexeme);
        token = get_token();

        void* ast = NULL;
        
        if (token == LPAREN)
        {
            if (check_duplicate(global_vars, name) == 1)
            {
                fprintf(stderr, "ERROR: Multiple declarations at LINE: %d\n", line);
                exit(1);
            }
            insert_id(&global_funcs, name, chk_decl_flag, -1);
            ast = func_defn(name);
            if (print_ast_flag)
            {
                print_ast(ast);
            }
        }

        else
        {
            if (check_duplicate(global_funcs, name) == 1)
            {
                fprintf(stderr, "ERROR: Multiple declarations at LINE: %d\n", line);
                exit(1);
            }
            insert_id(&global_vars, name, chk_decl_flag, -1);
            sym_add_global(name);
            var_decl(&global_vars, -1);
        }
        
        if (ast != NULL)
        {
            stack[count] = ast;
            count++;
        }
        
        free(name);
    }

    void* prog_ast = NULL;
    for (int i = count - 1; i >= 0; i--)
    {
        prog_ast = make_stmt_list(stack[i], prog_ast);
    }

    return prog_ast;
}

/* Calls a series of functions to match the syntax rules of a var_decl */
void var_decl(Identifier** list, int param) 
{
    id_list(list, param);
    match(SEMI);
}

/* Calls match for ID and COMMA for as long as the current token matches COMMA */
void id_list(Identifier** list, int param)
{
    while (token == COMMA)
    {
        match(COMMA);
        insert_id(list, lexeme, chk_decl_flag, param);
        if (param == -1)
        {
            sym_add_global(lexeme);
        }
        match(ID);
    }
}

/* Calls a series of functions to match the syntax rules of a func_defn */
void* func_defn(const char* name)
{
    local_vars = NULL;

    match(LPAREN);
    char** argnames;
    int nargs = opt_formals(&argnames);
    match(RPAREN);
    match(LBRACE);
    opt_var_decls();
    void* body = opt_stmt_list();
    match(RBRACE);

    void* ast = make_func_defn(name, nargs, argnames, body);

    int offset = 8;
    Identifier* cur = local_vars;
    while (cur)
    {
        if (cur->is_param == 1)
        {
            cur->offset = offset;
            offset += 4;
        }
        cur = cur->next;
    }

    offset = -4;
    cur = local_vars;
    while(cur)
    {
        if (cur->is_param == 0)
        {
            cur->offset = offset;
            offset -= 4;
        }
        cur = cur->next;
    }

    FuncInfo* f = malloc(sizeof(FuncInfo));
    if (!f)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    f->name = strdup(name);
    f->num_params = nargs;
    f->vars = NULL;
    cur = local_vars;
    while (cur)
    {
        if (cur->is_param == 1)
        {
            sym_add_param(f, cur->name, cur->offset);
        }
        else if (cur->is_param == 0)
        {
            sym_add_local(f, cur->name, cur->offset);
        }

        cur = cur->next;
    }

    f->next = sym_functions;
    sym_functions = f;

    free_list(&local_vars);
    return ast;
}

/* Calls match for kwINT at the start of a func_defn */
void type()
{
    match(kwINT);
}

/* Calls formals() for as long as the current token matches kwINT */
int opt_formals(char*** argnames)
{
    if (token == kwINT)
    {
        return formals(argnames);
    }
    *argnames = NULL;
    return 0;
}

/* Calls var_decl() for as long as the current token matches kwINT */
void opt_var_decls()
{
    while (token == kwINT)
    {
        token = get_token();
        if (token != ID)
        {
            report_error("Syntax error");
        }

        insert_id(&local_vars, lexeme, chk_decl_flag, 0);
        token = get_token();
        var_decl(&local_vars, 0);
    }
}

/* Calls type(), match for ID, and COMMA for as long as the current token matches COMMA */
int formals(char*** argnames)
{
    int count = 0;
    int cap = 4;
    char** args = malloc(cap*sizeof(char*));

    type();
    args[count++] = strdup(lexeme);
    insert_id(&local_vars, lexeme, chk_decl_flag, 1);
    match(ID);

    while (token == COMMA)
    {
        match(COMMA);
        type();
        if (count >= 4)
        {
            cap *= 2;
            args = realloc(args, cap*sizeof(char*));
        }

        args[count++] = strdup(lexeme);
        insert_id(&local_vars, lexeme, chk_decl_flag, 1);
        match(ID);
    }

    if (chk_decl_flag == 1 && global_funcs != NULL)
    {
        global_funcs->args = count;
    }

    *argnames = args;
    return count;
}

/* Calls stmt() for as long as the current token matches ID */
void* opt_stmt_list()
{
    void* stack[256];
    int count = 0;
    int loops = 0;
    int nonempty= 0;

    while (token == ID || token == kwWHILE || token == kwIF || token == kwRETURN || token == LBRACE || token == SEMI)
    {
        void* stmt_ast = stmt();
        loops++;
        if (stmt_ast != NULL)
        {
            nonempty = 1;
            stack[count++] = stmt_ast;
        }
    }

    void* list = NULL;
    for (int i = count - 1; i >= 0; i--)
    {
        list = make_stmt_list(stack[i], list);
    }

    if (loops > 1 || nonempty)
    {
        return make_stmt_list(NULL, list);
    }

    if (bracket_flag == 1)
    {
        bracket_flag = 0;
        return make_stmt_list(NULL, list);
    }

    return list;
}

/* Checks the next token and calls the appropriate function accordingly */
void* stmt()
{
    if (token == ID)
    {
        char* name = strdup(lexeme);
        token = get_token();
        void* ast = NULL;
        
        if (token == LPAREN)
        {
            ast = fn_call(name);
            match(SEMI);
        }
        else
        {
            if (chk_decl_flag == 1)
            {
                if (check_declared(name) == 0)
                {
                    fprintf(stderr, "ERROR: Undeclared variable at LINE %d\n", line);
                    exit(1);
                }
            }
            ast = assg_stmt(name);
        }
        free(name);
        return ast;
    }
    
    else if (token == kwWHILE)
    {
        return while_stmt();
    }

    else if (token == kwIF)
    {
        return if_stmt();
    }
    
    else if (token == kwRETURN)
    {
        return return_stmt();
    }

    else if (token == LBRACE)
    {
        match(LBRACE);
        void* body = opt_stmt_list();
        match(RBRACE);

        if (!body)
        {
            bracket_flag = 1;
            return NULL;
        }
        if (ast_node_type(body) != STMT_LIST)
        {
            return make_stmt_list(NULL, body);
        }

        return body;
    }

    else if (token == SEMI)
    {
        match(SEMI);
        return NULL;
    }

    else
    {
        report_error("Syntax error");
        return NULL;
    }
}

/* Calls a series of functions to match the syntax rules of a if_stmt */
void* if_stmt()
{
    match(kwIF);
    match(LPAREN);
    void* expr = bool_exp();
    match(RPAREN);

    void* then_branch = stmt();

    void* else_branch = NULL;
    if (token == kwELSE)
    {
        match(kwELSE);
        else_branch = stmt();
    }
    
    return make_if_stmt(expr, then_branch, else_branch);
}

/* Calls a series of functions to match the syntax rules of a while_stmt */
void* while_stmt()
{
    match(kwWHILE);
    match(LPAREN);
    void* expr = bool_exp();
    match(RPAREN);

    void* body = stmt();

    return make_while_stmt(expr, body);
}

/* Calls a series of functions to match the syntax rules of a return_stmt */
void* return_stmt()
{
    match(kwRETURN);
    void* expr = NULL;
    
    if (token != SEMI)
    {
        expr = arith_exp();
    }
    
    match(SEMI);
    return make_return_stmt(expr);
}

/* Calls a series of functions to match the syntax rules of a assg_stmt */
void* assg_stmt(const char* lhs)
{
    match(opASSG);
    void* rhs = arith_exp();
    match(SEMI);
    return make_assg_stmt(lhs, rhs);
}

/* Calls a series of functions to match the syntax rules of a fn_call */
void* fn_call(const char* name)
{
    if (chk_decl_flag == 1)
    {
        if (check_duplicate(local_vars, name) == 1)
        {
            fprintf(stderr, "ERROR: Multiple declarations at LINE: %d\n", line);
            exit(1);
        }

        if (check_duplicate(global_funcs, name) == 0)
        {
            fprintf(stderr, "ERROR: Undeclared function at LINE: %d\n", line);
            exit(1);
        }
    }

    match(LPAREN);
    void* args = opt_expr_list(name);
    match(RPAREN);

    return make_func_call(name, args);
}

/* Parse lowest precedence order operators */
void* parse_primary()
{
    if (token == ID)
    {
        char* name = strdup(lexeme);
        token = get_token();
        
        if (token == LPAREN)
        {
            void* call = fn_call(name);
            free(name);
            return call;
        }

        else
        {
            if (chk_decl_flag == 1)
            {
                if (check_declared(name) == 0)
                {
                    fprintf(stderr, "ERROR: Undeclared variable at LINE %d\n", line);
                    free(name);
                    exit(1);
                }
            }

            void* id = make_identifier(name);
            free(name);
            return id;
        }
    }
    
    else if (token == INTCON)
    {
        void* con = make_intconst(atoi(lexeme));
        match(INTCON);
        return con;
    }

    else if (token == LPAREN)
    {
        match(LPAREN);
        void* exp = arith_exp();
        match(RPAREN);
        return exp;
    }

    else
    {
        report_error("Syntax error");
        return NULL;
    }
}

/* Parse unary operator while ensuring correct precedence */
void* parse_unary()
{
    if (token == opSUB)
    {
        match(opSUB);
        void* rhs = parse_unary();
        return make_unary_expr(UMINUS, rhs);
    }

    return parse_primary();
}

/* Parse mul or div arithmetic operators while ensuring correct precedence */
void* parse_muldiv()
{
    void* left = parse_unary();
    while (token == opMUL || token == opDIV)
    {
        int op = token;
        match(op);
        void* right = parse_unary();

        NodeType ntype;
        if (op == opMUL)
        {
            ntype = MUL;
        }
        else if (op == opDIV)
        {
            ntype = DIV;
        }
        else
        {
            report_error("Invalid operator");
        }

        left = make_binary_expr(ntype, left, right);
    }

    return left;
}

/* Parse add or sub arithmetic operators while ensuring correct precedence */
void* parse_addsub()
{
    void* left = parse_muldiv();
    while (token == opADD || token == opSUB)
    {
        int op = token;
        match(op);
        void* right = parse_muldiv();

        NodeType ntype;
        if (op == opADD)
        {
            ntype = ADD;
        }
        else if (op == opSUB)
        {
            ntype = SUB;
        }
        else
        {
            report_error("Invalid operator");
        }

        left = make_binary_expr(ntype, left, right);
    }

    return left;
}

/* Parse relational boolean operators while ensuring correct precedence */
void* parse_relational()
{
    void* left = parse_addsub();
    if (token == opEQ || token == opNE || token == opLE || token == opLT || token == opGE || token == opGT)
    {
        NodeType ntype = relop();
        void* right = parse_addsub();
        return make_binary_expr(ntype, left, right);
    }

    return left;
}

/* Parse AND boolean operator while ensuring correct precedence */
void* parse_and()
{
    void* left = parse_relational();
    while (token == opAND)
    {
        int op = token;
        match(op);
        void* right = parse_relational();
        NodeType ntype = AND;
        left = make_binary_expr(ntype, left, right);
    }

    return left;
}

/* Parse OR boolean operator while ensuring correct precedence */
void* parse_or()
{
    void* left = parse_and();
    while (token == opOR)
    {
        int op = token;
        match(op);
        void* right = parse_and();
        NodeType ntype = OR;
        left = make_binary_expr(ntype, left, right);
    }

    return left;
}

/* Checks if the next token matches the requirements for expr_list or does nothing */
void* opt_expr_list(const char* name)
{
    void* stack[16];
    int count = 0;

    while (token == ID || token == INTCON || token == LPAREN || token == opSUB)
    {
        stack[count++] = arith_exp();
        if (token != COMMA)
        {
            break;
        }
        match(COMMA);
    }

    void* list = NULL;
    for (int i = count - 1; i >= 0; i--)
    {
        list = make_expr_list(stack[i], list);
    }

    if (chk_decl_flag == 1)
    {
        check_arguments(name, count);
    }

    return list;
}

/* Calls arith_exp and recurses if the next token is COMMA */
int expr_list()
{
    arith_exp();
    int count = 1;
    while (token == COMMA)
    {
        match(COMMA);
        arith_exp();
        count++;
    }

    return count;
}

/* Calls a series of functions to match the syntax rules of a bool_exp */
void* bool_exp()
{
    return parse_or();
}

/* Checks if the next token matches either ID or INTCON */
void* arith_exp()
{
    return parse_addsub();
}

/* Checks if the next token is a valid token for the rules for relop */
NodeType relop()
{
    NodeType op;

    if (token == opEQ)
    {
        op = EQ;
        match(opEQ);
    }

    else if (token == opNE)
    {
        op = NE;
        match(opNE);
    }
    
    else if (token == opLE)
    {
        op = LE;
        match(opLE);
    }
    
    else if (token == opLT)
    {
        op = LT;
        match(opLT);
    }
    
    else if (token == opGE)
    {
        op = GE;
        match(opGE);
    }
    
    else if (token == opGT)
    {
        op = GT;
        match(opGT);
    }
    
    else
    {
        report_error("Syntax Error");
    }

    return op;
}