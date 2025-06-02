#ifndef _QUANTA_INTERN_H
#define _QUANTA_INTERN_H

#include "atom.h"

struct atom *intern(const char *name, int is_keyword);

void init_intern_tables(void);
void cleanup_intern_tables(void);

// Used internally to mark interned atoms as part of GC mark phase
void intern_gc_mark(void);

#endif  // _QUANTA_INTERN_H
