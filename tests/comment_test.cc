#include <atom.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(CommentTest, OnlyComment) {
  struct source_file *source = source_file_str("; only a comment", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(CommentTest, TrailingComment) {
  struct source_file *source = source_file_str("5 ; only a comment", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 5);

  source_file_free(source);
}

TEST(CommentTest, InterleavedComment) {
  struct source_file *source = source_file_str("(5 . ; only a comment\n6)", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_cons(atom));
  EXPECT_TRUE(is_int(car(atom)));
  EXPECT_TRUE(is_int(cdr(atom)));
  EXPECT_EQ(car(atom)->value.ivalue, 5);
  EXPECT_EQ(cdr(atom)->value.ivalue, 6);

  source_file_free(source);
}
