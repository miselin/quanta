#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <print.h>
#include <read.h>
#include <source.h>

TEST(QuoteTests, BasicQuote) {
  struct source_file *source = source_file_str("(quote x)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  source_file_free(source);
}

TEST(QuoteTests, BasicUnquote) {
  struct source_file *source = source_file_str("(define x 5)\n(quasiquote (a b (unquote x)))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_cons(atom));

  struct atom *value = car(atom);
  EXPECT_TRUE(is_symbol(value));
  EXPECT_STREQ(value->value.string.ptr, "a");

  atom = cdr(atom);
  value = car(atom);

  EXPECT_TRUE(is_symbol(value));
  EXPECT_STREQ(value->value.string.ptr, "b");

  atom = cdr(atom);
  value = car(atom);

  EXPECT_TRUE(is_int(value));
  EXPECT_EQ(value->value.ivalue, 5);

  source_file_free(source);
}

TEST(QuoteTests, NestedQuote) {
  struct source_file *source =
      source_file_str("(define x 5)\n(quasiquote (a (quasiquote (b (unquote x)))))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  atom = eval(read_atom(source), env);

  // This one is frankly easier to compare as a printed string
  char buf[1024];
  print_str(buf, sizeof(buf), atom, 0);

  EXPECT_STREQ(buf, "(a (quasiquote (b (unquote x))))");

  source_file_free(source);
}

TEST(QuoteTests, UnquoteWithoutQuasiquote) {
  struct source_file *source = source_file_str("(unquote x)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}
