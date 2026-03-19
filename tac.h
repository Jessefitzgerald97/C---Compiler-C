#ifndef TAC_H
#define TAC_H

typedef enum {
    TAC_LABEL,
    TAC_GOTO,
    TAC_IFGOTO,
    TAC_ASSIGN,
    TAC_BINOP,
    TAC_UNARY,
    TAC_PARAM,
    TAC_CALL,
    TAC_SETRET,
    TAC_GETRET,
    TAC_ENTER,
    TAC_LEAVE,
    TAC_RETURN
} TACKind;

typedef struct TAC {
    TACKind kind;

    char* res;
    char* arg1;
    char* arg2;
    char* op;
    char* label;
    int nargs;

    struct TAC *next;
} TAC;

typedef struct TACList {
    TAC *head;
    TAC *tail;
} TACList;

TAC* new_tac(TACKind kind, const char* res, const char* arg1,
             const char* arg2, const char* op, const char* label, int nargs);

TACList* new_tac_list(void);
void tac_append(TACList *list, TAC *inst);
void tac_join(TACList* a, TACList* b);
void free_inst(TAC* t);
void free_tac_list(TACList* list);
void tac_print(const TACList* list);

TACList* gen_expr(void* expr, char** result);
TACList* gen_stmt(void* stmt);
TACList* gen_func(void* func_ast);
TACList* gen_program(void* ast_root);

char* new_temp();
char* new_label();

#endif
