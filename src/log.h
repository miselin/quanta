#ifndef _QUANTA_LOG_H
#define _QUANTA_LOG_H

#define LOGGER_MAIN 0
#define LOGGER_ATOM 1
#define LOGGER_ENV 2
#define LOGGER_EVAL 3
#define LOGGER_GC 4
#define LOGGER_INTERN 5
#define LOGGER_PRIM 6
#define LOGGER_SPEC 7
#define LOGGER_PRINT 8
#define LOGGER_READ 9
#define LOGGER_SOURCE 10
#define LOGGER_LEX 11

#define LOGGER_COUNT 12

#ifdef __cplusplus
extern "C" {
#endif

void logging_init(int to_file, int min_level);
void logging_shutdown(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _QUANTA_LOG_H
