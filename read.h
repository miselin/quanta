#ifndef _MATTLISP_READ_H
#define _MATTLISP_READ_H

#include <stdio.h>

#include "atom.h"
#include "source.h"

struct atom *read_atom(struct source_file *source);

#endif  // _MATTLISP_READ_H
