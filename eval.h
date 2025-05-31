#ifndef _MATTLISP_EVAL_H
#define _MATTLISP_EVAL_H

#include "atom.h"
#include "env.h"

struct atom *eval(struct atom *atom, struct environment *env);
struct atom *apply(struct atom *fn, struct atom *args, struct environment *env);

#endif  // _MATTLISP_EVAL_H
