#include "eval.h"

#include "atom.h"
#include "env.h"

struct atom *eval(struct atom *atom, struct environment *env) {
  (void)env;
  return atom;
}
