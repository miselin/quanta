#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(PrimitivesTest, CarMustBeNonEmptyList) {
  struct source_file *source = source_file_str("(car ())", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, CarMustBeList) {
  struct source_file *source = source_file_str("(car 1)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, CdrMustBeList) {
  struct source_file *source = source_file_str("(cdr 1)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, ConsCanBeNilNil) {
  struct source_file *source = source_file_str("(cons nil nil)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_cons(atom));
  EXPECT_TRUE(is_nil(car(atom)));
  EXPECT_TRUE(is_nil(cdr(atom)));

  source_file_free(source);
}

TEST(PrimitivesTest, EqualityIntegers) {
  struct source_file *source = source_file_str("(eq? 1234 1234)\n(eq? 1234 5678)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, EqualityFloats) {
  struct source_file *source = source_file_str("(eq? 3.5 3.5)\n(eq? 3.5 7.0)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, EqualityStrings) {
  struct source_file *source = source_file_str("(eq? \"abc\" \"abc\")\n(eq? \"abc\" \"def\")", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, EqualityTrue) {
  struct source_file *source = source_file_str("(eq? t t)\n(eq? t nil)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, EqualityPrimitive) {
  struct source_file *source = source_file_str("(eq? cons cons)\n(eq? cons car)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, EqualitySpecial) {
  struct source_file *source = source_file_str("(eq? set! set!)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}

TEST(PrimitivesTest, EqualityCons) {
  struct source_file *source =
      source_file_str("(define x (cons 1 2))\n(eq? x x)\n(eq? x (cons 1 2))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  // Identity equality passes - conses are the same object
  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_true(atom));

  // Identity equality fails - conses have same values, but aren't the same object
  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_nil(atom));

  source_file_free(source);
}
