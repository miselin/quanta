#ifndef _QUANTA_READ_H
#define _QUANTA_READ_H

#include <stdio.h>

#include "atom.h"
#include "source.h"

struct atom *read_atom(struct source_file *source);

#endif  // _QUANTA_READ_H
