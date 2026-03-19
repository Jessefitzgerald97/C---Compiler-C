#ifndef PARSER_H
#define PARSER_H

typedef struct Identifier 
{
    char* name;
    int args;
    int is_param;
    int offset;
    struct Identifier* next;
} Identifier;

typedef struct VarInfo {
    char* name;
    int offset;
    int is_param;
    struct VarInfo* next;
} VarInfo;

typedef struct FuncInfo {
    char* name;
    int num_params;
    VarInfo* vars;
    struct FuncInfo* next;
} FuncInfo;

#endif