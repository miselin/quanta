#ifndef _QUANTA_SOURCE_H
#define _QUANTA_SOURCE_H

#include <stddef.h>

struct source_file;

// Creates a new source file by opening the given file.
struct source_file *source_file_new(const char *filename);

// Creates a new source file from stdin.
struct source_file *source_file_stdin(void);

// Creates a new source file from the given string. The string is copied.
struct source_file *source_file_str(const char *str, size_t length);

char source_file_getc(struct source_file *source);
void source_file_ungetc(struct source_file *source, char c);

int source_file_eof(struct source_file *source);

void source_file_free(struct source_file *source);

#endif  // _QUANTA_SOURCE_H
