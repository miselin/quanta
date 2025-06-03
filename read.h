#ifndef _QUANTA_READ_H
#define _QUANTA_READ_H

#include "atom.h"
#include "source.h"

#ifdef __cplusplus
extern "C" {
#endif

struct atom *read_atom(struct source_file *source);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _QUANTA_READ_H
