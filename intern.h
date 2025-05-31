#ifndef _MATTLISP_INTERN_H
#define _MATTLISP_INTERN_H

#include "atom.h"

struct atom *intern(const char *name, int is_keyword);

void init_intern_tables(void);
void cleanup_intern_tables(void);

#endif  // _MATTLISP_INTERN_H
