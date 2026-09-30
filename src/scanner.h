#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "token.h"

int scanner_open(const char *path);

void scanner_close(void);

Token gettoken(void);

int scanner_error_count(void);

int scan_file(const char *path, FILE *out);

#endif