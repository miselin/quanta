#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "atom.h"
#include "env.h"
#include "eval.h"
#include "gc.h"
#include "intern.h"
#include "print.h"
#include "read.h"
#include "source.h"

int main(int argc, char *argv[]) {
  gc_init();
  init_intern_tables();

  struct source_file *source = NULL;
  if (argc > 1) {
    source = source_file_new(argv[1]);
    if (!source) {
      fprintf(stderr, "Error: could not open source file '%s'\n", argv[1]);
      return 1;
    }
  } else {
    source = source_file_stdin();
  }

  struct environment *env = create_default_environment();
  gc_retain(env);

  // fprintf(stderr, "repl: using environment %p\n", (void *)env);

  int is_interactive = isatty(STDIN_FILENO);

  while (1) {
    if (is_interactive) {
      printf("mattlisp> ");
      fflush(stdout);
    }

    if (source_file_eof(source)) {
      printf("End of input.\n");
      break;  // Exit on EOF
    }

    struct atom *atom = read_atom(source);
    if (!atom) {
      printf("End of input or error reading atom.\n");
      break;
    }

    struct atom *evaled = eval(atom, env);

    if (!evaled) {
      printf("Error evaluating atom.\n");
      continue;
    }

    print(stdout, evaled);
    printf("\n");

    gc_run();
  }

  source_file_free(source);

  fflush(stdout);

  gc_release(env);

  cleanup_intern_tables();

  gc_run();
  gc_shutdown();
  return 0;
}
