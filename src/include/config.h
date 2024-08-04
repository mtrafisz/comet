#ifndef _COMET_CONFIG_H
#define _COMET_CONFIG_H

#include <stdbool.h>
#include "logger.h"

void comet_init(bool very_verbose, bool log_with_colors) {
    verbose_output = very_verbose;
    log_use_colors = log_with_colors;
}

#endif
