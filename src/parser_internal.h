#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include <stdio.h>
#include "token.h"


extern Token cur;      
extern int   p_err;   
extern int   blamed;  
extern int   lex_err; 
extern FILE *pout;     

void advance(void);

void match(TokenType expected);

void emit(const char *message);

void blame(const char *message);

void Prg(void);
void Blk(void);
void Stm(void);
void Argfollow(void);
void Arg(void);
void Iffollow(void);
void Cnd(void);
void Rel(void);

void Exp(void);
void Trmfollow(void);
void Trm(void);
void Facfollow(void);
void Fac(void);
void Litfollow(void);
void Lit(void);
void Val(void);

#endif 
