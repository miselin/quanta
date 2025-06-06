#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(DefineTests, DefunSquare) {
  struct source_file *source = source_file_str("(defun square (x) (* x x))\n(square 5)", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "square");

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 25);

  source_file_free(source);
}

TEST(DefineTests, Closures) {
  struct source_file *source = source_file_str(
      "(define make-adder (lambda (x) (lambda (y) (+ x y))))\n(define add5 (make-adder 5))\n(add5 "
      "10)",
      0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();
  gc_retain(env);

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "make-adder");

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "add5");

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 15);

  gc_release(env);

  source_file_free(source);
}

TEST(DefineTests, MutableBindings) {
  struct source_file *source = source_file_str("(define x 1)\n(set! x 42)\nx", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 42);

  source_file_free(source);
}

TEST(DefineTests, RecursiveFunction) {
  struct source_file *source = source_file_str(
      "(defun factorial (n) "
      "  (cond ((eq? n 0) 1) "
      "        (t (* n (factorial (- n 1))))))"
      "\n(factorial 5)",
      0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "factorial");

  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 120);

  source_file_free(source);
}

TEST(DefineTests, Macros) {
  struct source_file *source = source_file_str(
      "(defmacro setcar! (place newval) `(set! ,place (cons ,newval (cdr ,place))))\n"
      "(define x (cons 0 1))\n"
      "(setcar! x 1)\n"
      "x",
      0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  // defmacro setcar!
  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "setcar!");

  // define x
  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  // setcar!
  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_symbol(atom));
  EXPECT_STREQ(atom->value.string.ptr, "x");

  // evalated x
  atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_cons(atom));
  EXPECT_TRUE(is_int(car(atom)));
  EXPECT_TRUE(is_int(cdr(atom)));
  EXPECT_EQ(car(atom)->value.ivalue, 1);
  EXPECT_EQ(cdr(atom)->value.ivalue, 1);

  source_file_free(source);
}
