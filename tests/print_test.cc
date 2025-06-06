#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(PrintTest, SimpleString) {
  struct source_file *source = source_file_str("(to-string \"hello\")", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "\"hello\"");

  source_file_free(source);
}

TEST(PrintTest, EscapedString1) {
  struct source_file *source = source_file_str("(to-string \"hel\\\"lo\")", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "\"hel\\\"lo\"");

  source_file_free(source);
}

TEST(PrintTest, EscapedString2) {
  struct source_file *source = source_file_str("(to-string \"hel\\\\lo\")", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "\"hel\\\\lo\"");

  source_file_free(source);
}

TEST(PrintTest, EscapedString3) {
  struct source_file *source = source_file_str("(to-string \"hel\\nlo\")", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "\"hel\\nlo\"");

  source_file_free(source);
}

TEST(PrintTest, EscapedString4) {
  struct source_file *source = source_file_str("(to-string \"hel\\tlo\")", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "\"hel\\tlo\"");

  source_file_free(source);
}

TEST(PrintTest, Keyword) {
  struct source_file *source = source_file_str("(to-string :keyword)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, ":keyword");

  source_file_free(source);
}

TEST(PrintTest, Float) {
  struct source_file *source = source_file_str("(to-string 3.14)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "3.140000");

  source_file_free(source);
}

TEST(PrintTest, ConsPair) {
  struct source_file *source = source_file_str("(to-string (cons 1 2)))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_string(atom));
  EXPECT_STREQ(atom->value.string.ptr, "(1 . 2)");

  source_file_free(source);
}

TEST(PrintTest, NotEnoughArguments) {
  struct source_file *source = source_file_str("(to-string)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(PrintTest, TooManyArguments) {
  struct source_file *source = source_file_str("(to-string 1 2 3)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}
