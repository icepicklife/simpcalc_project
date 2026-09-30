#include <string.h>
#include "token.h"

static const char *const NAMES[TOK_COUNT] = {

    "Identifier",
    "Number",
    "String",
    "Assign",
    "Semicolon",
    "Colon",
    "Comma",
    "LeftParen",
    "RightParen",
    "Plus",
    "Minus",
    "Multiply",
    "Divide",
    "Raise",
    "LessThan",
    "Equal",
    "GreaterThan",
    "LTEqual",
    "GTEqual",
    "NotEqual",
    "Print",
    "If",
    "Else",
    "Endif",
    "Sqrt",
    "And",
    "Or",
    "Not",
    "EndofFile",
    "LexicalError"

};

struct KeywordEntry {
    const char *word;
    TokenType type;
};

static const struct KeywordEntry KEYWORDS[] = {
    { "PRINT", TOK_PRINT },
    { "IF", TOK_IF },
    { "ELSE", TOK_ELSE },
    { "ENDIF", TOK_ENDIF },
    { "SQRT", TOK_SQRT },
    { "AND", TOK_AND },
    { "OR", TOK_OR },
    { "NOT", TOK_NOT },
    { NULL, TOK_IDENTIFIER },
};

const char *token_name(TokenType t) {
    
    if (t < 0 || t >= TOK_COUNT) {
        return "Unknown";
    }
    return NAMES[t];

}

TokenType keyword_lookup(const char *lexeme) {

    int i;
    for (i = 0; KEYWORDS[i].word != NULL; i++) {
        if (strcmp(KEYWORDS[i].word, lexeme) == 0) {
            return KEYWORDS[i].type;
        }
    }
    return TOK_IDENTIFIER;

}

