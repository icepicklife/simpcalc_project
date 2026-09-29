/* ==========================================================================
 * parser.h -- Public interface of the parser (syntax analyser) module.
 *
 * Owners: Member B (statements) and Member C (expressions)
 * ========================================================================== */
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

/* Parser Tester: parse `path` by pulling tokens from the scanner one at a
 * time through gettoken(), writing the recognition and error messages to
 * `out`, followed by the verdict line
 *
 *     <file> is a valid SimpCalc program
 *     <file> is not a valid SimpCalc program
 *
 * Returns 1 if the program is valid, 0 otherwise. */
int parse_file(const char *path, FILE *out);

#endif /* PARSER_H */
