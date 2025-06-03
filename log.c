#include "log.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "third_party/clog.h"

static int log_fd = -1;

void logging_init(int to_file) {
  int fd = 2;
  if (to_file) {
    unlink("quanta.log");
    fd = open("quanta.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    log_fd = fd;
  }

  for (size_t i = 0; i < LOGGER_COUNT; ++i) {
    clog_init_fd(i, fd);
    clog_set_level(i, CLOG_DEBUG);
  }
}

void logging_shutdown(void) {
  for (size_t i = 0; i < LOGGER_COUNT; ++i) {
    clog_free(i);
  }

  if (log_fd >= 0) {
    close(log_fd);
    log_fd = -1;
  }
}
