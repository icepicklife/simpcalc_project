#include "parser_internal.h"

void Exp(void)
{
    if (p_err) {
        return;
    }
    Trm();
    Trmfollow();
}

void Trmfollow(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_PLUS || cur.type == TOK_MINUS) {
        advance();
        Trm();
        Trmfollow();
    }
}

void Trm(void)
{
    if (p_err) {
        return;
    }
    Fac();
    Facfollow();
}

void Facfollow(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_MULTIPLY || cur.type == TOK_DIVIDE) {
        advance();
        Fac();
        Facfollow();
    }
}

void Fac(void)
{
    if (p_err) {
        return;
    }
    Lit();
    Litfollow();
}

void Litfollow(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_RAISE) {
        advance();
        Lit();
        Litfollow();
    }
}

void Lit(void)
{
    if (p_err) {
        return;
    }
    if (cur.type == TOK_MINUS) {
        advance();
    }
    Val();
}

void Val(void)
{
    if (p_err) {
        return;
    }

    switch (cur.type) {

    case TOK_IDENTIFIER:
        match(TOK_IDENTIFIER);
        break;

    case TOK_NUMBER:
        match(TOK_NUMBER);
        break;

    case TOK_SQRT:
        match(TOK_SQRT);
        match(TOK_LEFTPAREN);
        Exp();
        match(TOK_RIGHTPAREN);
        break;

    default:
        match(TOK_LEFTPAREN);
        Exp();
        match(TOK_RIGHTPAREN);
        break;
    }
}
