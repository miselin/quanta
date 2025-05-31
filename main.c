#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "atom.h"
#include "env.h"
#include "eval.h"
#include "intern.h"
#include "print.h"
#include "read.h"

int main(void) {
  init_intern_tables();

  struct environment *env = create_default_environment();

  while (1) {
    if (isatty(STDIN_FILENO)) {
      printf("mattlisp> ");
      fflush(stdout);
    }

    if (feof(stdin)) {
      printf("End of input.\n");
      break;  // Exit on EOF
    }

    struct atom *atom = read_atom(stdin);
    if (!atom) {
      printf("End of input or error reading atom.\n");
      break;
    }

    struct atom *evaled = eval(atom, env);
    if (!evaled) {
      printf("Error evaluating atom.\n");
      free(atom);
      continue;
    }

    print(evaled);
    printf("\n");

    if (evaled != atom) {
      free_atom(evaled);  // Free the evaluated atom if it's different
    }
    free_atom(atom);  // Free the atom after printing
  }

  fflush(stdout);

  free_environment(env);

  cleanup_intern_tables();
  return 0;
}
