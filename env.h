#ifndef _MATTLISP_ENV_H
#define _MATTLISP_ENV_H

struct environment;

struct environment *create_default_environment(void);
struct environment *create_environment(struct environment *parent);

struct environment *ref_environment(struct environment *env);
void deref_environment(struct environment *env);

struct environment *clone_environment(struct environment *env);

struct atom *env_lookup(struct environment *env, struct atom *symbol);

void env_bind(struct environment *env, struct atom *symbol, struct atom *value);

// Binds without ref-ing the value atom.
void env_bind_noref(struct environment *env, struct atom *symbol, struct atom *value);

#endif  // _MATTLISP_ENV_H
