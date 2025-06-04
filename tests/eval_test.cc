#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(EvalTest, ApplyFunction) {
  struct source_file *source = source_file_str("(apply + '(1 2 3 4))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 10);

  source_file_free(source);
}

TEST(EvalTest, EvalQuoted) {
  struct source_file *source = source_file_str("(eval '(+ 1 2))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 3);

  source_file_free(source);
}
