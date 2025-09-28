#ifndef UTIL_LOGGER_H
#define UTIL_LOGGER_H

#include <stdio.h>
#include <stdbool.h>

typedef enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3
} LogLevel;

typedef struct Logger {
    FILE *file;
    LogLevel level;
    char current_date[16];
    char directory[512];
} Logger;

bool logger_init(Logger *logger, const char *directory, LogLevel level);
void logger_close(Logger *logger);
void logger_log(Logger *logger, LogLevel level, const char *component, const char *message);
void logger_set_level(Logger *logger, LogLevel level);

#endif
