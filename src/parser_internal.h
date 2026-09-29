/* ==========================================================================
 * parser_internal.h -- Shared plumbing for the two parser source files.
 *
 * The recursive-descent procedures are split across two files so that two
 * people can work at the same time without touching the same file:
 *
 *   parser_stm.c  (Member B)  Prg Blk Stm Argfollow Arg Iffollow Cnd Rel
 *                             + the plumbing defined below + parse_file()
 *   parser_exp.c  (Member C)  Exp Trmfollow Trm Facfollow Fac Litfollow
 *                             Lit Val
 *
 * Both files include this header, so the mutual recursion between the two
 * halves (Stm -> Exp, Val -> Exp) resolves at link time.
 *
 * One-token lookahead discipline
 * ------------------------------
 *   `cur` always holds the token the current procedure must decide on.
 *   Every procedure returns with `cur` holding the first token AFTER the
 *   construct it consumed. Never call gettoken() directly -- use advance().
 *
 * Error discipline
 * ----------------
 *   Two behaviours, selected by PARSER_CANVAS_ERRORS in parser_stm.c:
 *
 *   PARSER_CANVAS_ERRORS 1 (default) reproduces the posted sample output --
 *   a single "Parse Error on line N: Tok Expected." line, then the parse
 *   stops, because every procedure opens with `if (p_err) return;` and
 *   unwinds silently.
 *
 *   PARSER_CANVAS_ERRORS 0 is the two-tier model described in the handout:
 *   Tier 1, the DETAIL, comes from match(): "Symbol expected". Tier 2, the
 *   BLAME, is one of the three construct-level messages -- "Invalid
 *   Statement", "Missing relational operator", "Incomplete if Statement" --
 *   printed by the INNERMOST procedure that owns the failure, via blame().
 *   blame() prints at most once per parse, so a syntax error deep inside
 *   nested IFs does not produce one "Invalid Statement" per nesting level.
 *
 *   The two-tier model exists because Blk only calls Stm when the lookahead
 *   is already in {Identifier, PRINT, IF}, so Stm can never be entered on a
 *   token it cannot start. If "Invalid Statement" were only printed in that
 *   dead branch it would never appear at all. The handout says to print it
 *   when "the Stm procedure fails", so Stm blames itself whenever its chosen
 *   production fails anywhere inside.
 * ========================================================================== */
#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include <stdio.h>
#include "token.h"

/* ---- shared state (defined in parser_stm.c) ----------------------------- */
extern Token cur;      /* the one-token lookahead                          */
extern int   p_err;    /* 1 once a syntax error has been reported          */
extern int   blamed;   /* 1 once a tier-2 construct-level message was sent */
extern int   lex_err;  /* 1 if the scanner reported any lexical error      */
extern FILE *pout;     /* where parser messages go                         */

/* ---- plumbing (defined in parser_stm.c) -------------------------------- */

/* Load the next token into `cur`. Lexical errors are reported and skipped
 * so that parsing can continue, but they still make the program invalid. */
void advance(void);

/* The match function required by the handout: verify that `cur` is the
 * expected terminal and consume it, otherwise report the mismatch. */
void match(TokenType expected);

/* Print one of the required messages ("Assignment Statement Recognized",
 * "If Statement Begins", ...). Does not set the error flag. */
void emit(const char *message);

/* Tier-2: print a construct-level error message ("Invalid Statement",
 * "Missing relational operator", "Incomplete if Statement") and set p_err.
 * Does nothing if some other procedure already blamed this failure, and
 * prints nothing at all under PARSER_CANVAS_ERRORS. */
void blame(const char *message);

/* ---- productions implemented by Member B in parser_stm.c --------------- */
void Prg(void);
void Blk(void);
void Stm(void);
void Argfollow(void);
void Arg(void);
void Iffollow(void);
void Cnd(void);
void Rel(void);

/* ---- productions implemented by Member C in parser_exp.c --------------- */
void Exp(void);
void Trmfollow(void);
void Trm(void);
void Facfollow(void);
void Fac(void);
void Litfollow(void);
void Lit(void);
void Val(void);

#endif /* PARSER_INTERNAL_H */
