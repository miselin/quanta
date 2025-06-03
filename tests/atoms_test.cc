#include <atom.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(AtomsTest, Quote) {
  struct source_file *source = source_file_str("(quote (1 2 3))", 0);
  ASSERT_TRUE(source != NULL);

  struct atom *atom = read_atom(source);
  EXPECT_TRUE(is_cons(atom));
  EXPECT_TRUE(is_symbol(car(atom)));
  EXPECT_STREQ(car(atom)->value.string.ptr, "quote");

  struct atom *quoted_list = car(cdr(atom));
  EXPECT_TRUE(is_cons(quoted_list));
  EXPECT_TRUE(is_int(car(quoted_list)));
  EXPECT_EQ(car(quoted_list)->value.ivalue, 1);
  quoted_list = cdr(quoted_list);
  EXPECT_TRUE(is_cons(quoted_list));
  EXPECT_TRUE(is_int(car(quoted_list)));
  EXPECT_EQ(car(quoted_list)->value.ivalue, 2);
  quoted_list = cdr(quoted_list);
  EXPECT_TRUE(is_cons(quoted_list));
  EXPECT_TRUE(is_int(car(quoted_list)));
  EXPECT_EQ(car(quoted_list)->value.ivalue, 3);
  quoted_list = cdr(quoted_list);
  EXPECT_TRUE(is_nil(quoted_list));

  source_file_free(source);
}
