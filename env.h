#ifndef _MATTLISP_ENV_H
#define _MATTLISP_ENV_H

struct environment;

struct environment *create_default_environment(void);
struct environment *create_environment(struct environment *parent);
void free_environment(struct environment *env);

struct environment *clone_environment(struct environment *env);

struct atom *env_lookup(struct environment *env, struct atom *symbol);
void env_bind(struct environment *env, struct atom *symbol, struct atom *value);

#endif  // _MATTLISP_ENV_H
