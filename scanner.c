/*
 * File: scanner.c
 * Author: Jack Fitzgerald
 * Purpose: Scanner that reads from stdin to identify and return tokens based on the tokens
 * and patterns given in the G0 language spec, ignoring comments and whitespace
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

#define MAX_LEN 1024

char* lexeme = NULL;
int line = 1;

/* 
* Iterates through the input as long as the current character is
* whitespace or part of a comment
*/
void ignore_whitespace() 
{
    int ch;
    while ((ch = getchar()) != EOF)
    {
        /* Ignore space characters */
        if(isspace(ch))
        {
            if (ch == '\n')
            {
                line++;
            }
            continue;
        }

        /* Ignore comments */
        if (ch == '/')
        {
            int next = getchar();
            if (next == '*')
            {
                int prev = 0;
                while ((ch = getchar()) != EOF)
                {
                    if (ch == '\n')
                    {
                        line++;
                    }
                    if (prev == '*' && ch == '/')
                    {
                        break;
                    }
                    prev = ch;
                }
                continue;
            }
            else
            {
                ungetc(next, stdin);
                ungetc(ch, stdin);
                return;
            }
        }
        else
        {
            ungetc(ch, stdin);
            return;
        }
    }
}

/* Checks if token string matches specific word tokens */
int is_word_token(char* str)
{
    if (strcmp(str, "int") == 0)
    {
        return kwINT;
    }
    
    if (strcmp(str, "if") == 0)
    {
        return kwIF;
    }
    
    if (strcmp(str, "else") == 0)
    {
        return kwELSE;
    }
    
    if (strcmp(str, "while") == 0)
    {
        return kwWHILE;
    }

    if (strcmp(str, "return") == 0)
    {
        return kwRETURN;
    }
    return UNDEF;
}

/*
* Iterates through stdin to identify and return token patterns
*/
int get_token()
{
    ignore_whitespace();
    
    int ch = getchar();
    if (ch == EOF)
    {
        return EOF;
    }
    static char buffer[MAX_LEN];
    int i = 0;
    buffer[0] = '\0';

    /* Checks for word tokens or identifiers */
    if (isalpha(ch)) 
    {
        buffer[i++] = ch;
        while (((ch = getchar()) != EOF) && (isalnum(ch) || ch == '_'))
        {
            if (i < MAX_LEN - 1)
            {
                buffer[i++] = ch;
            }
        }
        buffer[i] = '\0';
        if (ch != EOF)
        {
            ungetc(ch, stdin);
        }

        free(lexeme);
        lexeme = strdup(buffer);

        int word = is_word_token(lexeme);
        if (word != UNDEF)
        {
            return word;
        }
        else
        {
            return ID;
        }
    }

    /* Checks for integer constant tokens */
    if (isdigit(ch))
    {
        buffer[i++] = ch;
        while ((ch = getchar()) != EOF && isdigit(ch))
        {
            if (i < MAX_LEN - 1)
            {
                buffer[i++] = ch;
            }
        }
        buffer[i] = '\0';
        if (ch != EOF)
        {
            ungetc(ch, stdin);
        }
        
        free(lexeme);
        lexeme = strdup(buffer);

        return INTCON;
    }

    /* Check for two character tokens */
    int nextch = getchar();
    
    if (ch == '=' && nextch == '=')
    {
        free(lexeme);
        lexeme = strdup("==");
        return opEQ;
    }

    if (ch == '!' && nextch == '=')
    {
        free(lexeme);
        lexeme = strdup("!=");
        return opNE;
    }

    if (ch == '!' && nextch == '=')
    {
        free(lexeme);
        lexeme = strdup("!=");
        return opNE;
    }

    if (ch == '>' && nextch == '=')
    {
        free(lexeme);
        lexeme = strdup(">=");
        return opGE;
    }

    if (ch == '<' && nextch == '=')
    {
        free(lexeme);
        lexeme = strdup("<=");
        return opLE;
    }

    if (ch == '&' && nextch == '&')
    {
        free(lexeme);
        lexeme = strdup("&&");
        return opAND;
    }

    if (ch == '|' && nextch == '|')
    {
        free(lexeme);
        lexeme = strdup("||");
        return opOR;
    }

    if (nextch != EOF)
    {
        ungetc(nextch, stdin);
    }

    /* Check for single character tokens */
    switch(ch)
    {
        case '=': 
            free(lexeme); 
            lexeme = strdup("=");
            return opASSG;
        
        case '+': 
            free(lexeme); 
            lexeme = strdup("+");
            return opADD;

        case '-': 
            free(lexeme); 
            lexeme = strdup("-");
            return opSUB;

        case '*': 
            free(lexeme); 
            lexeme = strdup("*");
            return opMUL;
            
        case '/': 
            free(lexeme); 
            lexeme = strdup("/");
            return opDIV;
            
        case '>': 
            free(lexeme); 
            lexeme = strdup(">");
            return opGT;

        case '<': 
            free(lexeme); 
            lexeme = strdup("<");
            return opLT;

        case '!': 
            free(lexeme); 
            lexeme = strdup("!");
            return opNOT;

        case '(': 
            free(lexeme); 
            lexeme = strdup("(");
            return LPAREN;

        case ')': 
            free(lexeme); 
            lexeme = strdup(")");
            return RPAREN;
          
        case '{': 
            free(lexeme); 
            lexeme = strdup("{");
            return LBRACE;
            
        case '}': 
            free(lexeme); 
            lexeme = strdup("}");
            return RBRACE;
            
        case ',': 
            free(lexeme); 
            lexeme = strdup(",");
            return COMMA;
            
        case ';': 
            free(lexeme); 
            lexeme = strdup(";");
            return SEMI;   
    }

    /* Unknown token */
    buffer[0] = ch;
    buffer[1] = '\0';
    free(lexeme);
    lexeme = strdup(buffer);
    return UNDEF;
}