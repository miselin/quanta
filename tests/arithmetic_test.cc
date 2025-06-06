#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(ArithmeticTest, Add) {
  struct source_file *source = source_file_str("(+ 1 2)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 3);

  source_file_free(source);
}

TEST(ArithmeticTest, AddMixedTypes) {
  struct source_file *source = source_file_str("(+ 1 2.0)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ArithmeticTest, AddWrongTypes1) {
  struct source_file *source = source_file_str("(+ 1 \"string\")", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ArithmeticTest, AddWrongTypes2) {
  struct source_file *source = source_file_str("(+ \"string\" 1)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ArithmeticTest, Subtract) {
  struct source_file *source = source_file_str("(- 5 2)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 3);

  source_file_free(source);
}

TEST(ArithmeticTest, Multiply) {
  struct source_file *source = source_file_str("(* 3 4)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 12);

  source_file_free(source);
}

TEST(ArithmeticTest, Divide) {
  struct source_file *source = source_file_str("(/ 8 2)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 4);

  source_file_free(source);
}
