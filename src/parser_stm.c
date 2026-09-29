/* ==========================================================================
 * parser_stm.c -- Statement-level recursive descent, plus the shared
 *                 plumbing (cur / advance / match / emit / blame) and
 *                 the parse_file() driver.
 *
 * Owner: Member B
 *
 * Productions implemented here (from the handout's grammar):
 *
 *   Prg       -> Blk EndOfFile
 *   Blk       -> Stm Blk                       {Identifier, PRINT, IF}
 *              | eps
 *   Stm       -> Identifier := Exp ;           {Identifier}
 *              | PRINT ( Arg Argfollow ) ;     {PRINT}
 *              | IF Cnd : Blk Iffollow         {IF}
 *   Argfollow -> , Arg Argfollow               {Comma}
 *              | eps
 *   Arg       -> String                        {String}
 *              | Exp
 *   Iffollow  -> ENDIF ;                       {ENDIF}
 *              | ELSE Blk ENDIF ;              {ELSE}
 *   Cnd       -> Exp Rel Exp
 *   Rel       -> < | = | > | <= | >= | !=
 * ========================================================================== */
#include <string.h>

/* --------------------------------------------------------------------------
 * Which error reporting style to use.
 *
 * 1 (default) reproduces the POSTED SAMPLE OUTPUT byte for byte: a single
 *   "Parse Error on line N: Tok Expected." line, after which the parse
 *   stops. That is the graded reference, so it is the default.
 *
 * 0 is the two-tier model the handout's prose describes: "Symbol expected"
 *   from match(), plus exactly one construct-level blame line from the
 *   innermost procedure owning the failure. Nothing in the posted samples
 *   uses these strings; build with 0 only to demonstrate the handout's
 *   wording.
 * -------------------------------------------------------------------------- */
#define PARSER_CANVAS_ERRORS 1

#include "parser.h"
#include "parser_internal.h"
#include "scanner.h"

/* ---- shared state ------------------------------------------------------- */
Token cur;
int   p_err   = 0;
int   blamed  = 0;
int   lex_err = 0;
FILE *pout    = NULL;

/* ---- plumbing ----------------------------------------------------------- */

void advance(void)
{
    cur = gettoken();

    /* A lexical error is not a syntax error, so we report it, drop the bad
     * lexeme and keep going -- but the program can no longer be valid. */
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
        return;                 /* the innermost owner already reported */
    }
#if PARSER_CANVAS_ERRORS
    (void)message;              /* the samples report via match() instead */
#else
    fprintf(pout, "%s\n", message);
#endif
    blamed = 1;
    p_err  = 1;
}

/* The match function required by the handout. */
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

/* Expect `expected`, but blame `message` instead of reporting the mismatch
 * when it is missing. Used by Iffollow only, and only under the two-tier
 * model; the posted samples report these as ordinary parse errors. */
static void match_or(TokenType expected, const char *message)
{
    if (p_err) {
        return;
    }
    if (cur.type == expected) {
        advance();
    } else if (PARSER_CANVAS_ERRORS) {
        match(expected);        /* routes to the "Parse Error on line" line */
    } else {
        blame(message);
    }
}

/* ---- Prg -> Blk EndOfFile ---------------------------------------------- */
void Prg(void)
{
    if (p_err) {
        return;
    }
    Blk();
    /* Anything left over (a stray ENDIF, an extra ')', ...) lands here. */
    match(TOK_ENDOFFILE);
}

/* ---- Blk -> Stm Blk | eps ---------------------------------------------- */
void Blk(void)
{
    if (p_err) {
        return;
    }
    /* The valid set that can begin a statement. Any other token -- ELSE,
     * ENDIF, EndOfFile, or anything else -- takes Blk -> eps. */
    if (cur.type == TOK_IDENTIFIER || cur.type == TOK_PRINT || cur.type == TOK_IF) {
        Stm();
        Blk();                  /* right recursion, straight from the CFG */
    }
    /* else: epsilon production, consume nothing */
}

/* ---- Stm --------------------------------------------------------------- */
void Stm(void)
{
    if (p_err) {
        return;
    }

    switch (cur.type) {

    case TOK_IDENTIFIER:                    /* Identifier := Exp ; */
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

    case TOK_PRINT:                         /* PRINT ( Arg Argfollow ) ; */
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

    case TOK_IF:                            /* IF Cnd : Blk Iffollow */
        /* The handout says this message is printed at the START of the
         * procedure, so nested statements appear between Begins and Ends. */
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
        /* Defensive: unreachable while Blk guards the call with the three
         * valid sets, but kept so Stm is correct if called from anywhere. */
        blame("Invalid Statement");
        break;
    }
}

/* ---- Arg -> String | Exp ----------------------------------------------- */
void Arg(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_STRING) {
        match(TOK_STRING);
    } else {
        Exp();                  /* the production with no valid set */
    }
}

/* ---- Argfollow -> , Arg Argfollow | eps -------------------------------- */
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

/* ---- Iffollow -> ENDIF ; | ELSE Blk ENDIF ; ---------------------------- */
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
        /* Under the two-tier model any failure from here on is an error
         * inside Iffollow -> ELSE Blk ENDIF ; so the required message is
         * "Incomplete if Statement". */
        match_or(TOK_ENDIF, "Incomplete if Statement");
        match_or(TOK_SEMICOLON, "Incomplete if Statement");
        return;
    }

    /* Neither ENDIF nor ELSE: the if statement was never closed. */
    if (PARSER_CANVAS_ERRORS) {
        match(TOK_ENDIF);
    } else {
        blame("Incomplete if Statement");
    }
}

/* ---- Cnd -> Exp Rel Exp ------------------------------------------------ */
void Cnd(void)
{
    if (p_err) {
        return;
    }
    Exp();
    Rel();
    Exp();
}

/* ---- Rel -> < | = | > | <= | >= | != ----------------------------------- */
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
        advance();              /* all six are single terminals */
        break;
    default:
        /* Rel has no epsilon production either. */
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

/* ==========================================================================
 * parse_file -- the Parser Tester.
 * ========================================================================== */

/* Return just the file name part of a path, for the verdict line. */
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

    advance();                  /* prime the one-token lookahead */
    Prg();

    ok = (!p_err && !lex_err);
#if PARSER_CANVAS_ERRORS
    /* The posted samples print the verdict line only for a valid program:
     * every invalid sample ends at its "Parse Error on line ..." line. */
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
