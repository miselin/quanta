#include <atom.h>
#include <clog.h>
#include <gc.h>
#include <gtest/gtest.h>
#include <intern.h>
#include <log.h>
#include <read.h>
#include <source.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  logging_init(0, CLOG_INFO);
  gc_init();

  init_intern_tables();

  int rc = RUN_ALL_TESTS();

  cleanup_intern_tables();

  gc_run();
  gc_shutdown();
  logging_shutdown();
  return rc;
}
