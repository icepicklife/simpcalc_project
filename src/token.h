#ifndef TOKEN_H
#define TOKEN_H

#define MAX_LEXEME 1024

typedef enum {

    TOK_IDENTIFIER = 0,
    TOK_NUMBER,
    TOK_STRING,
    TOK_ASSIGN,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_COMMA,
    TOK_LEFTPAREN,
    TOK_RIGHTPAREN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_RAISE,
    TOK_LESSTHAN,
    TOK_EQUAL,
    TOK_GREATERTHAN,
    TOK_LTEQUAL,
    TOK_GTEQUAL,
    TOK_NOTEQUAL,

    TOK_PRINT,
    TOK_IF,
    TOK_ELSE,
    TOK_ENDIF,
    TOK_SQRT,
    TOK_AND,
    TOK_OR,
    TOK_NOT,

    TOK_ENDOFFILE,

    TOK_ERROR,
    TOK_COUNT
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[MAX_LEXEME];
    int line;
    const char *error;
} Token;

const char *token_name(TokenType t);

TokenType keyword_lookup(const char *lexeme);

#endif