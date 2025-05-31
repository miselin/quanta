#ifndef _MATTLISP_ATOM_H
#define _MATTLISP_ATOM_H

#include <stddef.h>
#include <stdint.h>

enum AtomType {
  ATOM_TYPE_INT = 0,      // 42
  ATOM_TYPE_FLOAT = 1,    // 3.14
  ATOM_TYPE_STRING = 2,   // "..."
  ATOM_TYPE_CONS = 3,     // lists
  ATOM_TYPE_NIL = 4,      // "nil"
  ATOM_TYPE_SYMBOL = 5,   // e.g. symbol (without quotes)
  ATOM_TYPE_KEYWORD = 6,  // e.g. :keyword
  ATOM_TYPE_TRUE = 7,     // t (nil is false)
};

struct atom {
  enum AtomType type;
  union {
    int64_t ivalue;
    double fvalue;
    struct {
      char *ptr;
      size_t len;
    } string;
    struct cons *cons;
  } value;
};

struct cons {
  struct atom *car;
  struct atom *cdr;
};

void free_atom(struct atom *atom);

#endif  // _MATTLISP_ATOM_H
