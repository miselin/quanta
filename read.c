#include "read.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "clog.h"
#include "intern.h"
#include "lex.h"
#include "log.h"

static struct atom *read_list(struct lex *lex);

static void consume_whitespace(struct source_file *source) {
  char c = source_file_getc(source);
  if (c == ';') {
    // skip comments
    while (c != EOF && c != '\n') {
      c = source_file_getc(source);
    }
  }

  while (c != EOF && isspace(c)) {
    c = source_file_getc(source);
  }

  if (c != EOF) {
    source_file_ungetc(source, c);
  }
}

static struct atom *read_atom_lex(struct lex *lex) {
  struct token *token = lex_next_token(lex);
  if (!token) {
    return new_atom_error(NULL, "could not read token from source");
  } else if (token->type == TOKEN_ERROR) {
    return new_atom_error(NULL, "could not read atom from source: %s", token->text);
  }

  switch (token->type) {
    case TOKEN_EOF:
      return new_atom_error(NULL, "unexpected end of file");
    case TOKEN_ERROR:
      return new_atom_error(NULL, "lexer error: %s", token->text);
    case TOKEN_ATOM:
      if (isdigit(token->text[0]) || (token->text[0] == '-' && isdigit(token->text[1]))) {
        // probably an integer or float
        char *endptr;
        long int_value = strtol(token->text, &endptr, 10);
        if (*endptr == '\0') {
          // it's an integer
          union atom_value value = {.ivalue = int_value};
          return new_atom(ATOM_TYPE_INT, value);
        } else {
          // try to parse as float
          double float_value = strtod(token->text, &endptr);
          if (*endptr == '\0') {
            union atom_value value = {.fvalue = float_value};
            return new_atom(ATOM_TYPE_FLOAT, value);
          } else {
            return new_atom_error(NULL, "could not parse number '%s'", token->text);
          }
        }
      }
      return intern(token->text, token->text[0] == ':');
    case TOKEN_STRING: {
      // TODO: handle escaped characters
      union atom_value value = {.string = {.ptr = strdup(token->text), .len = token->length - 2}};
      clog_debug(CLOG(LOGGER_READ), "read string: '%s'", value.string.ptr);
      return new_atom(ATOM_TYPE_STRING, value);
    }
    case TOKEN_LPAREN: {
      return read_list(lex);
    } break;
    case TOKEN_RPAREN:
      return new_atom_error(NULL, "unexpected right parenthesis");
    case TOKEN_QUOTE: {
      struct atom *atom = read_atom_lex(lex);
      if (is_error(atom)) {
        return atom;
      }

      struct atom *quote_atom = intern("quote", 0);
      return new_cons(quote_atom, new_cons(atom, atom_nil()));
    } break;
    case TOKEN_DOT:
      return new_atom_error(NULL, "unexpected dot in input, expected a list or atom");
  }

  return new_atom_error(NULL, "unknown token type: %d", token->type);
}

struct atom *read_atom(struct source_file *source) {
  struct lex *lex = lex_new(source);

  return read_atom_lex(lex);
}

static struct atom *read_list(struct lex *lex) {
  // LPAREN already consumed before this call

  struct atom *head = NULL;
  struct atom *prev = NULL;

  int dotted = 0;

  while (1) {
    struct token *token = lex_peek_token(lex);

    if (token->type == TOKEN_RPAREN) {
      // consume it
      lex_next_token(lex);
      break;
    } else if (token->type == TOKEN_DOT) {
      dotted = 1;
    } else if (token->type == TOKEN_ERROR) {
      return new_atom_error(NULL, "lexer error: %s", token->text);
    } else if (token->type == TOKEN_EOF) {
      return new_atom_error(NULL, "unexpected end of file while reading list");
    }

    // this will actually consume the token now
    struct atom *atom = read_atom_lex(lex);
    if (is_error(atom)) {
      return atom;
    }

    if (dotted) {
      if (!prev) {
        return new_atom_error(atom, "cannot have dotted pair without a previous cons cell");
      }

      if (!is_nil(cdr(prev))) {
        return new_atom_error(atom, "cannot have more than one dotted pair in a list");
      }

      prev->value.cons.cdr = atom;

      token = lex_next_token(lex);  // consume the closing parenthesis
      if (token->type != TOKEN_RPAREN) {
        return new_atom_error(atom, "expected ')', got '%s'", token->text);
      }

      break;
    }

    struct atom *cons = new_cons(atom, NULL);

    if (!head) {
      head = cons;  // first cons cell becomes the head of the list
    } else {
      prev->value.cons.cdr = cons;  // link the previous cons cell to the new one
    }

    prev = cons;  // update the previous pointer to the current cons cell
  }

  if (!head) {
    head = atom_nil();
  }

  return head;
}
