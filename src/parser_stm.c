#include <string.h>

#define PARSER_CANVAS_ERRORS 1

#include "parser.h"
#include "parser_internal.h"
#include "scanner.h"

Token cur;
int   p_err   = 0;
int   blamed  = 0;
int   lex_err = 0;
FILE *pout    = NULL;

void advance(void)
{
    cur = gettoken();

    while (cur.type == TOK_ERROR) {
        fprintf(pout, "Lexical Error (line %d): %s [%s]\n",
                cur.line, cur.error, cur.lexeme);
        lex_err = 1;
        cur = gettoken();
    }
}

void emit(const char *message)
{
    fprintf(pout, "%s\n", message);
}

void blame(const char *message)
{
    if (blamed) {
        return;                
    }
#if PARSER_CANVAS_ERRORS
    (void)message;            
#else
    fprintf(pout, "%s\n", message);
#endif
    blamed = 1;
    p_err  = 1;
}

void match(TokenType expected)
{
    if (p_err) {
        return;
    }
    if (cur.type == expected) {
        advance();
        return;
    }

#if PARSER_CANVAS_ERRORS
    fprintf(pout, "Parse Error on line %d: %s Expected.\n",
            cur.line, token_name(expected));
#else
    fprintf(pout, "Symbol expected\n");
    fprintf(pout, "    (expected %s, found %s '%s' on line %d)\n",
            token_name(expected), token_name(cur.type),
            cur.lexeme, cur.line);
#endif
    p_err = 1;
}

static void match_or(TokenType expected, const char *message)
{
    if (p_err) {
        return;
    }
    if (cur.type == expected) {
        advance();
    } else if (PARSER_CANVAS_ERRORS) {
        match(expected);       
    } else {
        blame(message);
    }
}

void Prg(void)
{
    if (p_err) {
        return;
    }
    Blk();
    match(TOK_ENDOFFILE);
}

void Blk(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_IDENTIFIER || cur.type == TOK_PRINT || cur.type == TOK_IF) {
        Stm();
        Blk();                 
    }
}

void Stm(void)
{
    if (p_err) {
        return;
    }

    switch (cur.type) {

    case TOK_IDENTIFIER:                  
        match(TOK_IDENTIFIER);
        match(TOK_ASSIGN);
        Exp();
        match(TOK_SEMICOLON);
        if (p_err) {
            blame("Invalid Statement");
        } else {
            emit("Assignment Statement Recognized");
        }
        break;

    case TOK_PRINT:                      
        match(TOK_PRINT);
        match(TOK_LEFTPAREN);
        Arg();
        Argfollow();
        match(TOK_RIGHTPAREN);
        match(TOK_SEMICOLON);
        if (p_err) {
            blame("Invalid Statement");
        } else {
            emit("Print Statement Recognized");
        }
        break;

    case TOK_IF:                           
        emit("If Statement Begins");
        match(TOK_IF);
        Cnd();
        match(TOK_COLON);
        Blk();
        Iffollow();
        if (p_err) {
            blame("Invalid Statement");
        } else {
            emit("If Statement Ends");
        }
        break;

    default:
        blame("Invalid Statement");
        break;
    }
}

void Arg(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_STRING) {
        match(TOK_STRING);
    } else {
        Exp();              
    }
}

void Argfollow(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_COMMA) {
        match(TOK_COMMA);
        Arg();
        Argfollow();
    }
}

void Iffollow(void)
{
    if (p_err) {
        return;
    }

    if (cur.type == TOK_ENDIF) {
        match(TOK_ENDIF);
        match(TOK_SEMICOLON);
        return;
    }

    if (cur.type == TOK_ELSE) {
        match(TOK_ELSE);
        Blk();
        match_or(TOK_ENDIF, "Incomplete if Statement");
        match_or(TOK_SEMICOLON, "Incomplete if Statement");
        return;
    }

    if (PARSER_CANVAS_ERRORS) {
        match(TOK_ENDIF);
    } else {
        blame("Incomplete if Statement");
    }
}

void Cnd(void)
{
    if (p_err) {
        return;
    }
    Exp();
    Rel();
    Exp();
}

void Rel(void)
{
    if (p_err) {
        return;
    }

    switch (cur.type) {
    case TOK_LESSTHAN:
    case TOK_EQUAL:
    case TOK_GREATERTHAN:
    case TOK_LTEQUAL:
    case TOK_GTEQUAL:
    case TOK_NOTEQUAL:
        advance();             
        break;
    default:
        if (PARSER_CANVAS_ERRORS) {
            fprintf(pout, "Parse Error on line %d: Relational Operator Expected.\n",
                    cur.line);
            p_err = 1;
        } else {
            blame("Missing relational operator");
        }
        break;
    }
}

static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');

#ifdef _WIN32
    const char *back = strrchr(path, '\\');
    if (back != NULL && (slash == NULL || back > slash)) {
        slash = back;
    }
#endif
    return (slash != NULL) ? slash + 1 : path;
}

int parse_file(const char *path, FILE *out)
{
    int ok;

    pout    = out;
    p_err   = 0;
    blamed  = 0;
    lex_err = 0;

    if (!scanner_open(path)) {
        fprintf(out, "Error: could not open %s\n", path);
        return 0;
    }

    advance();               
    Prg();

    ok = (!p_err && !lex_err);
#if PARSER_CANVAS_ERRORS
    if (ok) {
        fprintf(out, "%s is a valid SimpCalc program\n", base_name(path));
    }
#else
    fprintf(out, "%s is %s valid SimpCalc program\n",
            base_name(path), ok ? "a" : "not a");
#endif

    scanner_close();
    return ok;
}
