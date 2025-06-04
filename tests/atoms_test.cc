#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(AtomsTest, QuoteList) {
  struct source_file *source = source_file_str("(quote (1 2 3))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  struct atom *quoted_list = atom;
  EXPECT_TRUE(is_cons(quoted_list));

  struct atom *entry = car(quoted_list);
  EXPECT_TRUE(is_int(entry));
  EXPECT_EQ(entry->value.ivalue, 1);

  quoted_list = cdr(quoted_list);

  entry = car(quoted_list);
  EXPECT_TRUE(is_int(entry));
  EXPECT_EQ(entry->value.ivalue, 2);
  quoted_list = cdr(quoted_list);

  entry = car(quoted_list);
  EXPECT_TRUE(is_int(entry));
  EXPECT_EQ(entry->value.ivalue, 3);

  quoted_list = cdr(quoted_list);
  EXPECT_TRUE(is_nil(quoted_list));

  source_file_free(source);
}

TEST(AtomTest, QuoteSymbol) {
  struct source_file *source = source_file_str("(quote foo)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "foo");

  source_file_free(source);
}

TEST(AtomTest, IsQuotedSymbolAtom) {
  struct source_file *source = source_file_str("(atom? 'foo)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  source_file_free(source);
}

TEST(AtomTest, IsQuotedListAtom) {
  struct source_file *source = source_file_str("(atom? '(1 2 3))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}

TEST(AtomTest, IsEmptyListNil) {
  struct source_file *source = source_file_str("(nil? ())", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  source_file_free(source);
}

TEST(AtomTest, StringAtom) {
  struct source_file *source = source_file_str("\"atom\"", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "atom");

  source_file_free(source);
}

TEST(AtomTest, EscapedStringAtom) {
  struct source_file *source = source_file_str("\"at\\\"om\"", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "at\"om");

  source_file_free(source);
}
