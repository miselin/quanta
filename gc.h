#ifndef _QUANTA_GC_H
#define _QUANTA_GC_H

#include <stddef.h>

enum GCType {
  GC_TYPE_ATOM = 0,          // Regular atom
  GC_TYPE_ENVIRONMENT = 1,   // Environment
  GC_TYPE_BINDING_CELL = 2,  // Binding cell in environment
  GC_TYPE_TOKEN = 3,         // Lexer token
  GC_TYPE_LEXER = 4,         // Lexer state
};

void *gc_new(enum GCType type, size_t size);

void gc_retain(void *ptr);
void gc_release(void *ptr);

// Returns 1 if the pointer was already marked, 0 otherwise.
int gc_mark(void *ptr);

void gc_init(void);
void gc_run(void);
void gc_shutdown(void);

#endif  // _QUANTA_GC_H
