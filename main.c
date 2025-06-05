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
#include "log.h"
#include "print.h"
#include "read.h"
#include "source.h"
#include "third_party/clog.h"

int main(int argc, char *argv[]) {
  logging_init(1);

  gc_init();
  init_intern_tables();

  int is_interactive = 1;

  struct source_file *source = NULL;
  if (argc > 1) {
    source = source_file_new(argv[1]);
    if (!source) {
      fprintf(stderr, "Error: could not open source file '%s'\n", argv[1]);
      return 1;
    }

    is_interactive = 0;
  } else {
    source = source_file_stdin();
  }

  struct environment *env = create_default_environment();
  gc_retain(env);

  clog_debug(CLOG(LOGGER_MAIN), "REPL: using environment %p", (void *)env);

  while (1) {
    if (is_interactive) {
      printf("quanta> ");
      fflush(stdout);
    }

    if (source_file_eof(source)) {
      break;
    }

    struct atom *atom = read_atom(source);
    if (is_eof(atom)) {
      break;
    } else if (is_error(atom)) {
      fprintf(stderr, "Error reading atom: %s\n", atom->value.error.message);
      if (is_interactive) {
        continue;
      } else {
        break;
      }
    }

    struct atom *evaled = eval(atom, env);

    if (is_error(evaled)) {
      fprintf(stderr, "Error: %s\n", evaled->value.error.message);
      if (!is_interactive) {
        break;
      }
    } else if (is_interactive) {
      // Print the evaluated result, but only in interactive mode
      print(stdout, evaled, 0);
      printf("\n");
    }

    gc_run();
  }

  int rc = 0;

  if (source_file_eof(source)) {
    if (is_interactive) {
      printf("End of input.\n");
    }
  } else {
    rc = 1;
  }

  source_file_free(source);

  fflush(stdout);

  gc_release(env);

  cleanup_intern_tables();

  gc_run();
  gc_shutdown();
  logging_shutdown();
  return rc;
}
