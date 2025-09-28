#include "util/logger.h"
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#endif

static void logger_close_file(Logger *logger) {
    if (logger->file != NULL) {
        fclose(logger->file);
        logger->file = NULL;
    }
}

static bool logger_create_directory(const char *path) {
#ifdef _WIN32
    int result = _mkdir(path);
    if (result == 0 || errno == EEXIST) {
        return true;
    }
    return false;
#else
    int result = mkdir(path, 0755);
    if (result == 0 || errno == EEXIST) {
        return true;
    }
    return false;
#endif
}

static void logger_filename_for_today(char *buffer, size_t buffer_len) {
    time_t now = time(NULL);
    struct tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    strftime(buffer, buffer_len, "%Y-%m-%d", &tm_now);
}

bool logger_init(Logger *logger, const char *directory, LogLevel level) {
    char filename[1024];
    if (logger == NULL) {
        return false;
    }
    memset(logger, 0, sizeof(Logger));
    logger->level = level;
    strncpy(logger->directory, directory, sizeof(logger->directory) - 1);
    logger->directory[sizeof(logger->directory) - 1] = '\0';
    if (!logger_create_directory(directory)) {
        return false;
    }
    logger_filename_for_today(logger->current_date, sizeof(logger->current_date));
    snprintf(filename, sizeof(filename), "%s/%s.log", directory, logger->current_date);
    logger->file = fopen(filename, "a");
    if (logger->file == NULL) {
        return false;
    }
    return true;
}

void logger_set_level(Logger *logger, LogLevel level) {
    if (logger == NULL) {
        return;
    }
    logger->level = level;
}

static void logger_rotate_if_needed(Logger *logger) {
    char today[16];
    if (logger == NULL) {
        return;
    }
    logger_filename_for_today(today, sizeof(today));
    if (strcmp(today, logger->current_date) != 0) {
        char filename[1024];
        logger_close_file(logger);
        strncpy(logger->current_date, today, sizeof(logger->current_date) - 1);
        logger->current_date[sizeof(logger->current_date) - 1] = '\0';
        snprintf(filename, sizeof(filename), "%s/%s.log", logger->directory, logger->current_date);
        logger->file = fopen(filename, "a");
    }
}

static const char *logger_level_to_string(LogLevel level) {
    if (level == LOG_LEVEL_DEBUG) {
        return "DEBUG";
    }
    if (level == LOG_LEVEL_INFO) {
        return "INFO";
    }
    if (level == LOG_LEVEL_WARN) {
        return "WARN";
    }
    return "ERROR";
}

void logger_log(Logger *logger, LogLevel level, const char *component, const char *message) {
    time_t now;
    struct tm tm_now;
    char timestamp[32];
    if (logger == NULL || message == NULL) {
        return;
    }
    if (level < logger->level) {
        return;
    }
    logger_rotate_if_needed(logger);
    if (logger->file == NULL) {
        return;
    }
    now = time(NULL);
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &tm_now);
    fprintf(logger->file, "%s [%s] %s: %s\n", timestamp, logger_level_to_string(level), component == NULL ? "app" : component, message);
    fflush(logger->file);
}

void logger_close(Logger *logger) {
    if (logger == NULL) {
        return;
    }
    logger_close_file(logger);
}
