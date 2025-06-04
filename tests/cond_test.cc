#include <atom.h>
#include <env.h>
#include <eval.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <log.h>
#include <read.h>
#include <source.h>

TEST(ConditionalsTest, Equality) {
  struct source_file *source = source_file_str("(cond ((eq? 'a 'b) 1) ((eq? 'a 'a) 2) (t 3))", 0);
  ASSERT_TRUE(source != NULL);

  struct environment *env = create_default_environment();

  struct atom *atom = eval(read_atom(source), env);
  EXPECT_TRUE(is_int(atom));
  EXPECT_EQ(atom->value.ivalue, 2);

  source_file_free(source);
}
