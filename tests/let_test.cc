#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(LetTest, LetBinding) {
  struct source_file *source = source_file_str("(let ((x 2) (y 3)) (* x y))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 6);

  source_file_free(source);
}

TEST(LetTest, ShadowingVariables) {
  struct source_file *source = source_file_str("(let ((x 2)) (let ((x 3)) x))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 3);

  source_file_free(source);
}

TEST(LetTest, BindingReferences) {
  struct source_file *source = source_file_str("(let ((x 2) (y (* x 3))) y)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 6);

  source_file_free(source);
}

TEST(LetTest, NestedBindingReferences) {
  struct source_file *source = source_file_str("(let ((x 2)) (let ((y (* x 3))) y))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 6);

  source_file_free(source);
}
