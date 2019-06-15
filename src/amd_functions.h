#ifndef AMD_FUNCTIONS_H_
#define AMD_FUNCTIONS_H_

#include <stddef.h>
#include <stdint.h>

#include "tuxclocker-cli.h"
#include "../lib/libtuxclocker.h"

// Used in the main program to setup all AMD GPUs. Add the AMD GPU structs to gpu_list, remember to free the memory
int amd_setup_gpus(void *lib_handle, gpu **gpu_list, uint8_t *gpu_list_len);

// Used to get available tunables and limits. Return 0 if the tunable is available and a valid range was found.
int amd_get_tunable_range(tunable_valid_range *range, int tunable_enum, const char *hwmon_path_name, void *lib_handle, void *gpu_handle);
#endif
