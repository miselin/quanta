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

TEST(EvalTest, EvalPerformsTCO) {
  struct source_file *source = source_file_str(
      "(define sum (lambda (n acc)\n"
      "                    (cond ((eq? n 0) acc)\n"
      "                          (t (sum (- n 1) (+ n acc))))))\n"
      "(sum 10000 0)",
      0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "sum");

  atom = eval(read_atom(source), env);
  EXPECT_FALSE(is_error(atom));
  fprintf(stderr, "Result type: %s\n", atom_type_to_string(atom->type));
  if (is_error(atom)) {
    fprintf(stderr, "Error: %s\n", atom->value.error.message);
  }
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 50005000);

  source_file_free(source);
}
