#include "lex.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gc.h"
#include "log.h"
#include "source.h"
#include "third_party/clog.h"

struct lex {
  struct source_file *source;
  struct token *current_token;
  struct token eof;
};

static void lex_consume_whitespace(struct lex *lexer) {
  char c = source_file_getc(lexer->source);
  if (c == ';') {
    // skip comments
    while (c != EOF && c != '\n') {
      c = source_file_getc(lexer->source);
    }
  }

  while (c != EOF && isspace(c)) {
    c = source_file_getc(lexer->source);
  }

  if (c != EOF) {
    source_file_ungetc(lexer->source, c);
  }
}

// returns 1 if the character terminates an atom, 0 otherwise
static int is_terminator(char c) {
  return isspace(c) || c == '(' || c == ')' || c == ';' || c == '"' || c == '\'' || c == EOF;
}

static int read_until_terminator(struct lex *lexer, char terminator, char *buffer,
                                 size_t buffer_size, int allow_escaping) {
  int at = 0;
  int escape = 0;
  char c = source_file_getc(lexer->source);
  while (c != EOF && (escape || c != terminator)) {
    if (escape) {
      escape = 0;
    } else if (c == '\\' && allow_escaping) {
      escape = 1;
    }

    if ((size_t)at < buffer_size - 1) {
      buffer[at++] = c;
    }
    c = source_file_getc(lexer->source);
  }

  if (escape || c == EOF) {
    return -1;
  }

  buffer[at] = '\0';
  return at > 0 ? at : -1;
}

static int read_atom_string(struct lex *lexer, char *buffer, size_t buffer_size) {
  int at = 0;
  char c = source_file_getc(lexer->source);
  while (!is_terminator(c)) {
    if ((size_t)at < buffer_size - 1) {
      buffer[at++] = c;
    }
    c = source_file_getc(lexer->source);
  }
  source_file_ungetc(lexer->source, c);
  buffer[at] = '\0';
  return at > 0 ? at : -1;
}

struct lex *lex_new(struct source_file *source) {
  struct lex *lexer = gc_new(GC_TYPE_LEXER, sizeof(struct lex));
  lexer->source = source;
  lexer->current_token = NULL;

  lexer->eof.type = TOKEN_EOF;
  lexer->eof.text = NULL;

  return lexer;
}

struct token *lex_next_token(struct lex *lexer) {
  if (!lexer->current_token) {
    lex_peek_token(lexer);
    if (!lexer->current_token) {
      // we shouldn't get here - EOF and ERROR are both tokens in their own right
      return NULL;
    }
  }

  struct token *result = lexer->current_token;
  lexer->current_token = NULL;
  return result;
}

struct token *lex_peek_token(struct lex *lexer) {
  if (lexer->current_token) {
    return lexer->current_token;
  }

  if (source_file_eof(lexer->source)) {
    lexer->current_token = &lexer->eof;
    return lexer->current_token;
  }

  lex_consume_whitespace(lexer);

  char c = source_file_getc(lexer->source);
  if (c == EOF) {
    lexer->current_token = &lexer->eof;
    return lexer->current_token;
  }

  switch (c) {
    case '(':
    case ')': {
      char buf[2] = {c, '\0'};
      lexer->current_token = gc_new(GC_TYPE_TOKEN, sizeof(struct token));
      lexer->current_token->type = c == '(' ? TOKEN_LPAREN : TOKEN_RPAREN;
      lexer->current_token->text = strdup(buf);
      lexer->current_token->length = 1;
    } break;

    case '\'': {
      lexer->current_token = gc_new(GC_TYPE_TOKEN, sizeof(struct token));
      lexer->current_token->type = TOKEN_QUOTE;
      lexer->current_token->text = strdup("'");
      lexer->current_token->length = 1;
    } break;

    case '.': {
      lexer->current_token = gc_new(GC_TYPE_TOKEN, sizeof(struct token));
      lexer->current_token->type = TOKEN_DOT;
      lexer->current_token->text = strdup(".");
      lexer->current_token->length = 1;
    } break;

    case '"': {
      lexer->current_token = gc_new(GC_TYPE_TOKEN, sizeof(struct token));
      lexer->current_token->type = TOKEN_STRING;
      lexer->current_token->text = (char *)malloc(256);
      int length = read_until_terminator(lexer, '"', (char *)lexer->current_token->text, 256, 1);
      if (length < 0) {
        lexer->current_token->type = TOKEN_ERROR;
        lexer->current_token->text = strdup("error reading string literal");
        lexer->current_token->length = strlen(lexer->current_token->text);
      } else {
        lexer->current_token->length = length;
      }
    } break;

    default:
      source_file_ungetc(lexer->source, c);

      lexer->current_token = gc_new(GC_TYPE_TOKEN, sizeof(struct token));
      lexer->current_token->type = TOKEN_ATOM;
      lexer->current_token->text = (char *)malloc(256);
      int length = read_atom_string(lexer, (char *)lexer->current_token->text, 256);
      if (length < 0) {
        lexer->current_token->type = TOKEN_ERROR;
        lexer->current_token->text = strdup("error reading atom");
        lexer->current_token->length = strlen(lexer->current_token->text);
      } else if (length == 0) {
        lexer->current_token->type = TOKEN_ERROR;
        lexer->current_token->text = strdup("empty atom");
        lexer->current_token->length = strlen(lexer->current_token->text);
      } else {
        lexer->current_token->length = length;
      }
  }

  return lexer->current_token;
}

void lex_gc_erase(struct lex *lexer) {
  (void)lexer;
}

void lex_gc_erase_token(struct token *token) {
  if (token->text) {
    free((void *)token->text);
    token->text = NULL;
  }
  token->length = 0;
  token->type = TOKEN_EOF;
}

void lex_gc_mark(struct lex *lexer) {
  if (!lexer) {
    return;
  }

  if (lexer->current_token == &lexer->eof) {
    return;
  }

  gc_mark(lexer);
  gc_mark(lexer->current_token);
}
