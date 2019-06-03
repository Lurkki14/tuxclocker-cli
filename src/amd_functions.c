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

	/*
	int (*amd_get_gpu_fds)(uint8_t*, int**, size_t) = dlsym(lib_handle, "tc_amd_get_gpu_fds");
	int *fds = malloc(sizeof(int) * MAX_GPUS - *gpu_list_len);

	uint8_t amd_gpu_amount = 0;
	int retval = amd_get_gpu_fds(&amd_gpu_amount, &fds, MAX_GPUS - *gpu_list_len);
	if (retval != 0) {
		free(fds);
		return 1;
	}	

	// Get the handles
	int (*amd_get_handle_by_fd)(int, size_t, void*) = dlsym(lib_handle, "tc_amd_get_gpu_handle_by_fd");
	uint8_t valid_gpus_len = 0;
	for (uint8_t i=0; i<amd_gpu_amount; i++) {		
	        void *handle = malloc(MAX_STRUCT_SIZE);	       
		retval = amd_get_handle_by_fd(fds[i], MAX_STRUCT_SIZE - *gpu_list_len - valid_gpus_len, handle); 
		if (retval != 0)
			continue;
				
		valid_gpus_len++;
		gpu_list[*gpu_list_len + valid_gpus_len - 1] = malloc(sizeof(gpu));
		gpu_list[*gpu_list_len + valid_gpus_len - 1]->amd_handle = handle;
	       	gpu_list[*gpu_list_len + valid_gpus_len - 1]->fd = fds[i];
		gpu_list[*gpu_list_len + valid_gpus_len - 1]->gpu_type = AMD;	
	}
	*/
	/*
	// Allocate memory for sysfs paths	 
	char **paths = malloc(sizeof(char*) * valid_gpus_len);
	for (uint8_t i=0; i<valid_gpus_len; i++) { 
		gpu_list[*gpu_list_len + i]->hwmon_path = malloc(sizeof(char) * MAX_STRLEN); 
		paths[i] = malloc(sizeof(char) * MAX_STRLEN);
	}
		
	// Get the sysfs paths	
	int (*amd_get_hwmon_paths)(char***, size_t, size_t) = dlsym(lib_handle, "tc_amd_get_hwmon_paths");
	retval = amd_get_hwmon_paths(&paths, valid_gpus_len, MAX_STRLEN);
	if (retval != 0) {
		// Failed due to conflicting GPU amounts
		// Free memory		
		for (uint8_t i=0; i<valid_gpus_len; i++)
			free(paths[i]);
		
		free(fds);
		free(paths);
		return 1;
	}
	// Copy the hwmon paths to the gpu struct
	for (uint8_t i=0; i<valid_gpus_len; i++) {
		snprintf(gpu_list[*gpu_list_len + i]->hwmon_path, MAX_STRLEN, "%s", paths[i]);
		// Free memory
		free(paths[i]);
	}
	free(paths);	
	*/
	//free(fds);
	*gpu_list_len += amd_gpu_count;
	return 0;
}
