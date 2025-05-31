#ifndef _MATTLISP_READ_H
#define _MATTLISP_READ_H

#include <stdio.h>

#include "atom.h"

struct atom *read_atom(FILE *fp);

#endif  // _MATTLISP_READ_H
