#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "scanner.h"

#define STRICT_NUMBER_SUFFIX 1
#define STRING_KEEP_QUOTES 1

#define PB_EMPTY (-2)

typedef enum {
    Q0_START = 0,
    Q1_ID,
    Q2_WHOLE,
    Q3_DOT,
    Q4_FRAC,
    Q5_EXP_MARK,
    Q6_EXP_SIGN,
    Q7_EXP_DIGITS,
    Q8_STRING,
    Q9_SLASH,
    Q10_COMMENT,
    Q11_STAR,
    Q12_COLON,
    Q13_BANG,
    Q14_LT,
    Q15_GT
} State;

static FILE *src = NULL;
static int line_no = 1;
static int pushed = PB_EMPTY;
static int err_count = 0;
static int at_eof = 0;

static int nextch(void) {

    int c;

    if (pushed != PB_EMPTY) {
        c = pushed;
        pushed = PB_EMPTY;
    } else if (src == NULL) {
        c = EOF;
    } else {
        c = fgetc(src);
    }

    if (c == '\n') {
        line_no++;
    }

    return c;

}

static void pushback(int c) {
    if (c == EOF) {
        return;
    }
    if (c == '\n') {
        line_no--;
    }
    pushed = c;
}

static int is_letter(int c) {

    return c != EOF && isalpha((unsigned char) c);

}

static int is_digit(int c) {

    return c != EOF && isdigit((unsigned char) c);

}

static int is_space(int c) {

    return c == ' ' || c == '\t' || c == '\n' || c =='\v' || c == '\f' || c == '\r';

}

static void append(char *buf, int *len, int c) {

    if (*len < MAX_LEXEME - 1) {
        buf[(*len)++] = (char) c;
        buf[*len] = '\0';
    }

}

static Token make_token(TokenType type, const char *lexeme, int line) {

    Token t;

    t.type = type;
    t.line = line;
    t.error = NULL;

    if (lexeme == NULL) {
        t.lexeme[0] = '\0';
    } else {
        strncpy(t.lexeme, lexeme, MAX_LEXEME - 1);
        t.lexeme[MAX_LEXEME - 1] = '\0';
    }

    return t;

}

static Token make_error(const char *reason, const char *lexeme, int line) {
    
    Token t = make_token(TOK_ERROR, lexeme, line);

    t.error = reason;
    err_count++;
    return t;

}

int scanner_open(const char *path) {

    scanner_close();
    src = fopen(path, "r");
    line_no = 1;
    pushed = PB_EMPTY;
    err_count = 0;
    at_eof = 0;
    return src != NULL;

}

void scanner_close(void) {
    
    if (src != NULL) {
        fclose(src);
        src = NULL;
    }

}

int scanner_error_count(void) {
    
    return err_count;

}

Token gettoken(void) {

    char lex[MAX_LEXEME];
    int len = 0;
    int start_line = line_no;
    State st = Q0_START;
    int c;

    lex[0] = '\0';

    for (;;) {
        c = nextch();

        switch (st) {

            case Q0_START:

                start_line = line_no;
                len = 0;
                lex[0] = '\0';

                if (c == EOF) {
                    at_eof = 1;
                    return make_token(TOK_ENDOFFILE, "", start_line);
                }
                if (is_space(c)) {
                    break;
                }
                if (is_letter(c) || c == '_') {
                    append(lex, &len, c);
                    st = Q1_ID;
                    break;
                }
                if (is_digit(c)) {
                    append(lex, &len, c);
                    st = Q2_WHOLE;
                    break;
                }

                switch (c) {

                    case '"':

                        #if STRING_KEEP_QUOTES
                            append(lex, &len, c);
                        #endif
                        st = Q8_STRING;
                        break;

                    case '/':
                        st = Q9_SLASH;
                        break;
                    case '*':
                        st = Q11_STAR;
                        break;
                    case ':':
                        st = Q12_COLON;
                        break;
                    case '!':
                        st = Q13_BANG;
                        break;
                    case '<':
                        st = Q14_LT;
                        break;
                    case '>':
                        st = Q15_GT;
                        break;
                    case '=':
                        return make_token(TOK_EQUAL, "=", start_line);
                    case ';':
                        return make_token(TOK_SEMICOLON, ";", start_line);
                    case ',':
                        return make_token(TOK_COMMA, ",", start_line);
                    case '(':
                        return make_token(TOK_LEFTPAREN, "(", start_line);
                    case ')':
                        return make_token(TOK_RIGHTPAREN, ")", start_line);
                    case '+':
                        return make_token(TOK_PLUS, "+", start_line);
                    case '-':
                        return make_token(TOK_MINUS, "-", start_line);

                    default:
                        append(lex, &len, c);
                        return make_error("Illegal Sequence", lex, start_line);
                }
                break;
            
            case Q1_ID:
                if (is_letter(c) || is_digit(c) || c == '_') {
                    append(lex, &len, c);
                    break;
                }
                pushback(c);
                return make_token(keyword_lookup(lex), lex, start_line);

            case Q2_WHOLE:
                if (is_digit(c)) {
                    append(lex, &len, c);
                    break;
                }
                if (c == '.') {
                    append(lex, &len, c);
                    st = Q3_DOT;
                    break;
                }
                if (c == 'e' || c == 'E') {
                    append(lex, &len, c);
                    st = Q5_EXP_MARK;
                    break;
                }
                #if STRICT_NUMBER_SUFFIX
                    if (is_letter(c) || c == '_') {
                        pushback(c);
                        return make_error("Invalid Number", lex, start_line);
                    }
                #endif
                pushback(c);
                return make_token(TOK_NUMBER, lex, start_line);
            
            case Q3_DOT:
                if (is_digit(c)) {
                    append(lex, &len, c);
                    st = Q4_FRAC;
                    break;
                }
                pushback(c);
                return make_error("Invalid Number", lex, start_line);

            case Q4_FRAC:
                if (is_digit(c)) {
                    append(lex, &len, c);
                    break;
                }
                if (c == 'e' || c == 'E') {
                    append(lex, &len, c);
                    st = Q5_EXP_MARK;
                    break;
                }
                #if STRICT_NUMBER_SUFFIX
                    if (is_letter(c) || c == '_' || c == '.') {
                        pushback(c);
                        return make_error("Invalid Number", lex, start_line);
                    }
                #endif
                pushback(c);
                return make_token(TOK_NUMBER, lex, start_line);

            case Q5_EXP_MARK:
                if (c == '+' || c == '-') {
                    append(lex, &len, c);
                    st = Q6_EXP_SIGN;
                    break;
                }
                if (is_digit(c)) {
                    append(lex, &len, c);
                    st = Q7_EXP_DIGITS;
                    break;
                }
                pushback(c);
                return make_error("Invalid Number", lex, start_line);

            case Q6_EXP_SIGN:

                if (is_digit(c)) {
                    append(lex, &len, c);
                    st = Q7_EXP_DIGITS;
                    break;
                }
                pushback(c);
                return make_error("Invalid Number", lex, start_line);
            
            case Q7_EXP_DIGITS:
                if (is_digit(c)) {
                    append(lex, &len, c);
                    break;
                }
                #if STRICT_NUMBER_SUFFIX
                    if (is_letter(c) || c == '_' || c == '.') {
                        pushback(c);
                        return make_error("Invalid Number", lex, start_line);
                    }
                #endif
                pushback(c);
                return make_token(TOK_NUMBER, lex, start_line);

            case Q8_STRING:
                if (c == EOF || c == '\n') {
                    pushback(c);
                    return make_error("Unterminated String", lex, start_line);
                }
                if (c == '"') {
                    #if STRING_KEEP_QUOTES
                        append(lex, &len, c);
                    #endif
                    return make_token(TOK_STRING, lex, start_line);
                }
                append(lex, &len, c);
                break;

            case Q9_SLASH:
                if (c == '/') {
                    st = Q10_COMMENT;
                    break;
                }
                pushback(c);
                return make_token(TOK_DIVIDE, "/", start_line);
            
            case Q10_COMMENT:
                if (c == EOF) {
                    st = Q0_START;
                    pushback(c);
                    break;
                }
                if (c == '\n') {
                    st = Q0_START;
                    break;
                }
                break;
            
            case Q11_STAR:
                if (c == '*' ) {
                    return make_token(TOK_RAISE, "**", start_line);
                }
                pushback(c);
                return make_token(TOK_MULTIPLY, "*", start_line);
            
            case Q12_COLON:
                if (c == '=') {
                    return make_token(TOK_ASSIGN, ":=", start_line);
                }
                pushback(c);
                return make_token(TOK_COLON, ":", start_line);

            case Q13_BANG:
                if (c == '=') {
                    return make_token(TOK_NOTEQUAL, "!=", start_line);
                }
                pushback(c);
                return make_error("Invalid ! Character", "!", start_line);

            case Q14_LT:
                if (c == '=') {
                    return make_token(TOK_LTEQUAL, "<=", start_line);
                }
                pushback(c);
                return make_token(TOK_LESSTHAN, "<", start_line);

            case Q15_GT:
                if (c == '=') {
                    return make_token(TOK_GTEQUAL, ">=", start_line);
                }
                pushback(c);
                return make_token(TOK_GREATERTHAN, ">", start_line);

                    
        }
    }
}

#define SCAN_LINE_FMT "%s %s\n"

int scan_file (const char *path, FILE *out) {

    Token t;
    int errors;

    if (!scanner_open(path)) {
        fprintf(out, "Error: could not open %s\n", path);
        return 1;
    }

    for (;;) {
        t = gettoken();

        if (t.type == TOK_ERROR) {
            fprintf(out, "Lexical Error (line %d): %s [%s]\n", t.line, t.error, t.lexeme);
        } else if (t.type == TOK_ENDOFFILE) {
            fprintf(out, "%s\n", token_name(TOK_ENDOFFILE));
            break;
        } else {
            fprintf(out, SCAN_LINE_FMT, token_name(t.type), t.lexeme);
        }
    }

    errors = scanner_error_count();
    scanner_close();
    (void)at_eof;
    
    return errors;
}