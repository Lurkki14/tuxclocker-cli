#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>
#include <stdint.h>

#include "../lib/libtuxclocker.h"

// Return true if module containing module_name is loaded
bool is_module_loaded(const char *module_name);

char *get_value_and_unit_string(sensor_info info, const char *name, const char *unit);

#endif
