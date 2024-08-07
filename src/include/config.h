#ifndef _COMET_CONFIG_H
#define _COMET_CONFIG_H

#include <stdbool.h>
#include "logger.h"

/** 
 * @brief Configure the logger with the given verbosity and color settings.
 * 
 * For now it basically only removes color from terminal output if the user
 * doesn't want it.
 * 
 * @param very_verbose Whether to log everything or not.
 * @param log_with_colors Whether to log with colors or not.
 */
void comet_init(bool very_verbose, bool log_with_colors);

#endif
