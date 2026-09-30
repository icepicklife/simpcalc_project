#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "scanner.h"

#define MAX_PATH_LEN 1024
#define MAX_FILES    256

static void append_str(char *dst, size_t cap, size_t *len, const char *s)
{
    while (*s != '\0' && *len + 1 < cap) {
        dst[(*len)++] = *s++;
    }
    dst[*len] = '\0';
}

static void build_output_path(const char *in_path, const char *suffix,
                              char *out, size_t cap)
{
    char        stem[MAX_PATH_LEN];
    const char *dot;
    const char *slash;
    char       *found;
    size_t      stem_len;
    size_t      len = 0;

    dot   = strrchr(in_path, '.');
    slash = strrchr(in_path, '/');
#ifdef _WIN32
    const char *back = strrchr(in_path, '\\');
    if (back != NULL && (slash == NULL || back > slash)) {
        slash = back;
    }
#endif
    if (dot != NULL && (slash == NULL || dot > slash)) {
        stem_len = (size_t)(dot - in_path);
    } else {
        stem_len = strlen(in_path);
    }
    if (stem_len >= sizeof(stem)) {
        stem_len = sizeof(stem) - 1;
    }
    memcpy(stem, in_path, stem_len);
    stem[stem_len] = '\0';

    out[0] = '\0';

    found = strstr(stem, "input");
    if (found != NULL) {
        *found = '\0';
        append_str(out, cap, &len, stem);
        append_str(out, cap, &len, "output_");
        append_str(out, cap, &len, suffix);
        append_str(out, cap, &len, found + 5);
    } else {
        append_str(out, cap, &len, stem);
        append_str(out, cap, &len, "_output_");
        append_str(out, cap, &len, suffix);
    }
    append_str(out, cap, &len, ".txt");
}

static int process_file(const char *path)
{
    char  scan_path[MAX_PATH_LEN];
    char  parse_path[MAX_PATH_LEN];
    FILE *fp;
    int   lex_errors;
    int   valid;

    build_output_path(path, "scan",  scan_path,  sizeof(scan_path));
    build_output_path(path, "parse", parse_path, sizeof(parse_path));

    fp = fopen(scan_path, "w");
    if (fp == NULL) {
        fprintf(stderr, "Cannot write %s\n", scan_path);
        return 0;
    }
    lex_errors = scan_file(path, fp);
    fclose(fp);

    fp = fopen(parse_path, "w");
    if (fp == NULL) {
        fprintf(stderr, "Cannot write %s\n", parse_path);
        return 0;
    }
    valid = parse_file(path, fp);
    fclose(fp);

    printf("%-28s -> %-34s %-34s  [%d lexical error%s, %s]\n",
           path, scan_path, parse_path,
           lex_errors, (lex_errors == 1) ? "" : "s",
           valid ? "valid" : "not valid");
    return valid;
}

static int ends_with(const char *s, const char *suffix)
{
    size_t ls = strlen(s);
    size_t lt = strlen(suffix);

    return (ls >= lt) && (strcmp(s + ls - lt, suffix) == 0);
}

static int is_input_candidate(const char *name)
{
    if (!ends_with(name, ".txt")) {
        return 0;
    }
    if (strstr(name, "output") != NULL) {
        return 0;
    }
    return 1;
}

static int compare_names(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int process_directory(const char *dir)
{
    DIR           *d;
    struct dirent *entry;
    char          *names[MAX_FILES];
    int            count = 0;
    int            i;
    int            processed = 0;

    d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "Cannot open directory %s\n", dir);
        return 0;
    }

    while ((entry = readdir(d)) != NULL && count < MAX_FILES) {
        if (is_input_candidate(entry->d_name)) {
            char full[MAX_PATH_LEN];

            if (strcmp(dir, ".") == 0) {
                snprintf(full, sizeof(full), "%s", entry->d_name);
            } else {
                snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);
            }
            names[count] = malloc(strlen(full) + 1);
            if (names[count] == NULL) {
                break;
            }
            strcpy(names[count], full);
            count++;
        }
    }
    closedir(d);

    if (count == 0) {
        printf("No SimpCalc input files (*.txt) found in %s\n", dir);
        return 0;
    }

    qsort(names, (size_t)count, sizeof(names[0]), compare_names);

    for (i = 0; i < count; i++) {
        process_file(names[i]);
        free(names[i]);
        processed++;
    }
    return processed;
}

int main(int argc, char **argv)
{
    int i;
    int count = 0;

    if (argc >= 3 && strcmp(argv[1], "-d") == 0) {
        return (process_directory(argv[2]) > 0) ? 0 : 1;
    }

    if (argc == 1) {
        return (process_directory(".") > 0) ? 0 : 1;
    }

    for (i = 1; i < argc; i++) {
        process_file(argv[i]);
        count++;
    }
    return (count > 0) ? 0 : 1;
}
