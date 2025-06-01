#ifndef _MATTLISP_INTERN_H
#define _MATTLISP_INTERN_H

#include "atom.h"

struct atom *intern(const char *name, int is_keyword);

// intern but do not increment the reference count (used for creating default environments)
struct atom *intern_noref(const char *name, int is_keyword);

void init_intern_tables(void);
void cleanup_intern_tables(void);

#endif  // _MATTLISP_INTERN_H
