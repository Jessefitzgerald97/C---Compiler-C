#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "tac.h"

void emit_globals(void);
void emit_println(void);
void gen_code(const TACList* list);

#endif