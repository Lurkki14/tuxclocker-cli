#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "amd_functions.h"
//#include "../lib/libtuxclocker.h"

int amd_setup_gpus(void *lib_handle, gpu **gpu_list, uint8_t *gpu_list_len) {
	// Get file descriptors and hwmon paths
	char **hwmon_paths = NULL;
	int *fd_arr = NULL;
	uint8_t amd_gpu_count = 0;
	int (*amd_get_fs_info)(char***, int**, uint8_t*) = dlsym(lib_handle, "tc_amd_get_fs_info_all");
	int retval = amd_get_fs_info(&hwmon_paths, &fd_arr, &amd_gpu_count);
	if (retval != 0)
		return 1;

	int (*amd_get_handle_by_fd)(int, size_t, void*) = dlsym(lib_handle, "tc_amd_get_gpu_handle_by_fd");	
	for (uint8_t i=0; i<amd_gpu_count; i++) {
		// Get the GPU handle
		void *handle = malloc(MAX_STRUCT_SIZE);
		retval = amd_get_handle_by_fd(fd_arr[i], MAX_STRUCT_SIZE, handle);

		gpu_list[*gpu_list_len + i] = malloc(sizeof(gpu));
		gpu_list[*gpu_list_len + i]->hwmon_path = hwmon_paths[i];
		gpu_list[*gpu_list_len + i]->fd = fd_arr[i];
		gpu_list[*gpu_list_len + i]->amd_handle = handle;
		gpu_list[*gpu_list_len + i]->gpu_type = AMD;
	}
	*gpu_list_len += amd_gpu_count;
	return 0;
}

int amd_get_tunable_range(tunable_valid_range *range, int tunable_enum,
		const char *hwmon_path_name, void *lib_handle, void *gpu_handle) {
	switch (tunable_enum) {
		case TUNABLE_FAN_SPEED_PERCENTAGE: ;
			// Get the fan speed - if it can be read it can be changed
			int (*get_fan_speed)(void*, int*, int, const char*) = dlsym(lib_handle, "tc_amd_get_gpu_sensor_value");
			int reading;
			if (get_fan_speed(gpu_handle, &reading, tunable_enum, hwmon_path_name) == 0) {
				// Success
				range->min = 0;
				range->max = 100;
				range->tunable_value_type = TUNABLE_ABSOLUTE;
				return 0;
			}
			return 1;
		case TUNABLE_POWER_LIMIT: ;
			int (*get_power_limit_range)(int, const char*, tunable_valid_range*) = dlsym(lib_handle, "tc_amd_get_tunable_range");
			if (get_power_limit_range(tunable_enum, hwmon_path_name, range) == 0) {
				// Success
				return 0;
			}
			return 1;
		default: return 1;
	}
	return 1;
}
