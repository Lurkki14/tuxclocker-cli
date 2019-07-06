#ifndef LIBTUXCLOCKER_NVIDIA_H
#define LIBTUXCLOCKER_NVIDIA_H

#include <stdint.h>

#include "../libtuxclocker.h"
#include "../include/nvml.h"

// Get the amount of nvidia GPUs from NVML. Returns an nvmlReturn_t, which is an enum and 0 indicates success. Needs to be the first function called since it initializes NVML.
int tc_nvidia_get_gpu_count(uint8_t *gpu_count);

// Get the NVML device handle. Allocates memory and needs to be freed by the caller.
int tc_nvidia_get_nvml_handle_by_index(void **nvml_handle, int index);

// Get the X display that is used as a handle in xnvctrl. Allocates memory and needs to be freed by the caller.
int tc_nvidia_get_nvctrl_handle(void **nvctrl_handle);

// Get the GPU name from nvml
int tc_nvidia_get_nvml_gpu_name(void *nvml_handle, char (*buf)[], size_t str_len);

// Get sensor readings by handle. Return the data in info to support arbitrary types
int tc_nvidia_get_sensor_value(void *nvml_handle, void *nvctrl_handle, sensor_info *info, int sensor_enum, int gpu_idnex);

// Get the limits for a tunable. The pstate argument is used for mem/core clock offsets
int tc_nvidia_get_tunable_range(void *nvml_handle, void *nvctrl_handle, tunable_valid_range *range, int tunable_enum, int gpu_index, int pstate_index);

// Get the amount of performance states for a GPU.
int tc_nvidia_get_pstate_count(void *nvml_handle, int *pstate_count);
#endif
