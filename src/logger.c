#include "include/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

bool log_use_colors = true;
bool verbose_output = false;

const char* log_level_to_string(enum LogLevel level) {
    const char* level_strings[] = {
        "[DEBUG]",
        "[INFO] ",
        "[WARN] ",
        "[ERROR]",
    };
    const char* level_strings_color[] = {
        "[\033[1;36mDEBUG\033[0m]",
        "[\033[1;32mINFO\033[0m] ",
        "[\033[1;33mWARN\033[0m] ",
        "[\033[1;31mERROR\033[0m]",
    };

    if (log_use_colors) {
        return level_strings_color[level];
    } else {
        return level_strings[level];
    }
}

void log_message(enum LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);

    const char* format_string = "[%d-%02d-%02d %02d:%02d:%02d] %s ";
    const char* format_string_color = "[\033[0;35m%d-%02d-%02d %02d:%02d:%02d\033[0m] %s ";

    const char* format_string_final = log_use_colors ? format_string_color : format_string;

    printf(format_string_final,
           now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday,
           now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec,
           log_level_to_string(level));
    vprintf(format, args);
    printf("\n");

    va_end(args);
}
