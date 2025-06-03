#include "source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct source_file {
  int in_memory;

  union {
    struct {
      FILE *fp;
      int is_owned;
    } file;

    struct {
      char *buf;
      size_t buflen;
      size_t pos;
    } memory;
  } source;
};

struct source_file *source_file_new(const char *filename) {
  struct source_file *source = calloc(1, sizeof(struct source_file));
  source->in_memory = 0;
  source->source.file.fp = fopen(filename, "r");
  source->source.file.is_owned = 1;
  if (!source->source.file.fp) {
    perror("Failed to open file");
    free(source);
    return NULL;
  }

  return source;
}

struct source_file *source_file_str(const char *str, size_t length) {
  struct source_file *source = calloc(1, sizeof(struct source_file));
  source->in_memory = 1;
  source->source.memory.buf = strdup(str);
  source->source.memory.buflen = length == 0 ? strlen(str) : length;
  source->source.memory.pos = 0;

  return source;
}

struct source_file *source_file_stdin(void) {
  struct source_file *source = calloc(1, sizeof(struct source_file));
  source->in_memory = 0;
  source->source.file.fp = stdin;
  source->source.file.is_owned = 0;

  return source;
}

char source_file_getc(struct source_file *source) {
  if (source->in_memory) {
    if (source->source.memory.pos >= source->source.memory.buflen) {
      return EOF;  // End of string
    }
    return source->source.memory.buf[source->source.memory.pos++];
  } else {
    return fgetc(source->source.file.fp);
  }
}

void source_file_ungetc(struct source_file *source, char c) {
  if (source->in_memory) {
    if (source->source.memory.pos > 0) {
      source->source.memory.pos--;

      // Technically we have to actually store the character here.
      // ungetc could be used to insert a character back to the stream.
      // Could replace this with a separate "unget" buffer if needed.
      source->source.memory.buf[source->source.memory.pos] = c;
    }
  } else {
    ungetc(c, source->source.file.fp);
  }
}

int source_file_eof(struct source_file *source) {
  if (source->in_memory) {
    return source->source.memory.pos >= source->source.memory.buflen;
  } else {
    return feof(source->source.file.fp);
  }
}

void source_file_free(struct source_file *source) {
  if (!source) {
    return;
  }

  if (source->in_memory) {
    free(source->source.memory.buf);
  } else {
    fclose(source->source.file.fp);
  }

  free(source);
}
