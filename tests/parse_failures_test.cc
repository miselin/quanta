#include <atom.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(ParseFailuresTest, EmptyInput) {
  struct source_file *source = source_file_str("", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_eof(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, UnterminatedList1) {
  struct source_file *source = source_file_str("(", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, UnterminatedList2) {
  struct source_file *source = source_file_str("(1", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, UnterminatedList3) {
  struct source_file *source = source_file_str("(1 2 3", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, StrayRParen) {
  struct source_file *source = source_file_str(")", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, UnterminatedString) {
  struct source_file *source = source_file_str("\"str", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, EofInQuote) {
  struct source_file *source = source_file_str("'", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, InvalidCons1) {
  struct source_file *source = source_file_str("(1 . 2 . 3)", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, InvalidCons2) {
  struct source_file *source = source_file_str("(1 . )", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, InvalidDotAtStart) {
  struct source_file *source = source_file_str(".", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, InvalidDotInList) {
  struct source_file *source = source_file_str("(1 . . 2)", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, StrayQuote) {
  struct source_file *source = source_file_str("''", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, UnterminatedStringWithEscape) {
  struct source_file *source = source_file_str("\"abc\\", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}

TEST(ParseFailuresTest, OnlyWhitespace) {
  struct source_file *source = source_file_str("   \t\n  ", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_error(atom));

  source_file_free(source);
}
