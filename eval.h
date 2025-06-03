#ifndef _QUANTA_EVAL_H
#define _QUANTA_EVAL_H

#include "atom.h"
#include "env.h"

#ifdef __cplusplus
extern "C" {
#endif

struct atom *eval(struct atom *atom, struct environment *env);
struct atom *apply(struct atom *fn, struct atom *args, struct environment *env);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _QUANTA_EVAL_H
