#ifndef _QUANTA_ENV_H
#define _QUANTA_ENV_H

struct environment;

#ifdef __cplusplus
extern "C" {
#endif

struct environment *create_default_environment(void);
struct environment *create_environment(struct environment *parent);

struct environment *clone_environment(struct environment *env);

// Frees resources associated with an environment (e.g. hash tables).
// Does not free atoms referenced in the environment, GC will handle that.
void erase_environment(struct environment *env);

struct atom *env_lookup(struct environment *env, struct atom *symbol);

void env_bind(struct environment *env, struct atom *symbol, struct atom *value);
void env_set(struct environment *env, struct atom *symbol, struct atom *value);

void environment_gc_mark(struct environment *env);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _QUANTA_ENV_H
