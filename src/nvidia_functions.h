#ifndef NVIDIA_FUNCTIONS_H
#define NVIDIA_FUNCTIONS_H

#include <stdint.h>

#include "tuxclocker-cli.h"
#include "../lib/libtuxclocker.h"

// Assign the nvidia GPU handles
int nvidia_setup_gpus(void *lib_handle, gpu **gpu_list, uint8_t *gpu_list_len);

#endif
