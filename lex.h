#ifndef _QUANTA_LEX_H
#define _QUANTA_LEX_H

#include <stddef.h>

#include "source.h"

enum Token {
  TOKEN_EOF = 0,
  TOKEN_ERROR = 1,
  // A symbol, keyword, or number
  TOKEN_ATOM = 2,
  // A quoted string literal, including the surrounding quotes
  TOKEN_STRING = 3,
  // Left paren
  TOKEN_LPAREN = 4,
  // Right paren
  TOKEN_RPAREN = 5,
  // The ' character, used for quoting
  TOKEN_QUOTE = 6,
  // The . character, used for dotted pairs in lists
  TOKEN_DOT = 7,
};

struct token {
  enum Token type;
  const char *text;
  size_t length;
};

struct lex;

struct lex *lex_new(struct source_file *source);

// Consumes the next token in the token stream
// Returns the same as lex_peek_token if it was previously called, but advances the lexer
struct token *lex_next_token(struct lex *lexer);

// Peeks the next token in the token stream
struct token *lex_peek_token(struct lex *lexer);

void lex_gc_erase(struct lex *lexer);
void lex_gc_erase_token(struct token *token);
void lex_gc_mark(struct lex *lexer);

#endif  // _QUANTA_LEX_H
