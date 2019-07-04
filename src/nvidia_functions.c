#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lib/libtuxclocker.h"
#include "tuxclocker-cli.h"

int nvidia_setup_gpus(void *lib_handle, gpu **gpu_list, uint8_t *gpu_list_len) {
	// Get the amount of nvidia gpus
	int (*nvidia_get_gpu_count)(uint8_t*) = dlsym(lib_handle, "tc_nvidia_get_gpu_count");
	uint8_t nv_gpu_count = 0;
	int retval = nvidia_get_gpu_count(&nv_gpu_count);
	if (retval != 0) {
		return 1;
	}

	// Get the handles
	int (*nv_get_nvml_handle)(void**, int) = dlsym(lib_handle, "tc_nvidia_get_nvml_handle_by_index");
	int (*nv_get_nvctrl_handle)(void**) = dlsym(lib_handle, "tc_nvidia_get_nvctrl_handle");
	for (uint8_t i=0; i<nv_gpu_count; i++) {
		// Only fail if both functions fail since some functionality is available without one or the other
		void *nvml_handle = NULL, *nvctrl_handle = NULL;
		int nvml_retval = 0, nvctrl_retval = 0;
		nvml_retval = nv_get_nvml_handle(&nvml_handle, i);
		//nvctrl_retval = nv_get_nvctrl_handle(&nvctrl_handle);
		
		if (nvml_retval != 0 && nvctrl_retval != 0) {
			// Couldn't get either handle
			continue;
		}
		/*int (*get_name)(void*, int, size_t) = dlsym(lib_handle, "tc_nvidia_get_nvml_gpu_name");
		char name[64];
		get_name(nvml_handle, &name, 64);
		printf("%s\n", name);*/

		// Add the GPU to the list
		gpu_list[*gpu_list_len] = malloc(sizeof(gpu));
		gpu_list[*gpu_list_len]->nvml_handle = nvml_handle;
		gpu_list[*gpu_list_len]->nvctrl_handle = nvctrl_handle;
		gpu_list[*gpu_list_len]->gpu_type = NVIDIA;
		*gpu_list_len += 1;
	}
	return 0;
}
