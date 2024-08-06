#include "include/config.h"

void comet_init(bool very_verbose, bool log_with_colors) {
    verbose_output = very_verbose;
    log_use_colors = log_with_colors;
}
