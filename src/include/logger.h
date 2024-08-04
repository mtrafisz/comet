#ifndef _COMET_LOGGER_H
#define _COMET_LOGGER_H

#include <stdbool.h>
extern bool log_use_colors;
extern bool verbose_output;

enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

void log_message(enum LogLevel level, const char* format, ...);

#endif