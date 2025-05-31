#ifndef _MATTLISP_PRIMITIVE_H
#define _MATTLISP_PRIMITIVE_H

#include "atom.h"
#include "env.h"

struct atom *primitive_add(struct atom *args, struct environment *env);
struct atom *primitive_subtract(struct atom *args, struct environment *env);
struct atom *primitive_multiply(struct atom *args, struct environment *env);
struct atom *primitive_divide(struct atom *args, struct environment *env);

struct atom *primitive_equal(struct atom *args, struct environment *env);
struct atom *primitive_not_equal(struct atom *args, struct environment *env);
struct atom *primitive_less_than(struct atom *args, struct environment *env);
struct atom *primitive_greater_than(struct atom *args, struct environment *env);
struct atom *primitive_less_than_equal(struct atom *args, struct environment *env);
struct atom *primitive_greater_than_equal(struct atom *args, struct environment *env);

struct atom *primitive_cons(struct atom *args, struct environment *env);
struct atom *primitive_car(struct atom *args, struct environment *env);
struct atom *primitive_cdr(struct atom *args, struct environment *env);

void init_primitives(struct environment *env);

#endif  // _MATTLISP_PRIMITIVE_H
