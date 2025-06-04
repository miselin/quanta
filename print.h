#ifndef _QUANTA_PRINT_H
#define _QUANTA_PRINT_H

#include <stdio.h>

#include "atom.h"

#ifdef __cplusplus
extern "C" {
#endif

void print(FILE *fp, struct atom *atom, int readably);

// Set readably to 1 for human readable output
int print_str(char *buffer, size_t buffer_size, struct atom *atom, int readably);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _QUANTA_PRINT_H
