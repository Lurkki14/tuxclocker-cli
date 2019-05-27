#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>

#include "libtuxclocker_amd.h"
int *tc_amd_get_gpu_fds(uint8_t *len) {
	const char *dev_dir_name = "/dev/dri";
	// Try to initialize all renderD* devices in /dev/dri
	DIR *dev_dir;
	struct dirent *dev_entry;
	char dev_abs_path[128];
	int *fd_arr = NULL;

	// Open the directory
	dev_dir = opendir(dev_dir_name);
	if (dev_dir == NULL)
		// Couldn't open the directory
		return NULL;		

	// Try to open the files containing renderD
	int fd = 0;
	uint8_t amount = 0;
	uint32_t major, minor;
	while ((dev_entry = readdir(dev_dir)) != NULL) {
		if (strstr(dev_entry->d_name, "renderD") != NULL) {
			sprintf(dev_abs_path, "%s/%s", dev_dir_name, dev_entry->d_name);

			fd = open(dev_abs_path, O_RDONLY);
			if (fd < 1)
				continue;
			// Create a device handle and try to initialize 
			amdgpu_device_handle dev_handle;
			if (amdgpu_device_initialize(fd, &major, &minor, &dev_handle) == 0) {
				// Success
				amount++;
				fd_arr = realloc(fd_arr, sizeof(uint8_t) * amount);
				fd_arr[amount - 1] = fd;
			}
		}
	}
	*len = amount;
	return fd_arr;
}

void *tc_amd_get_gpu_handle_by_fd(int fd) {
	uint32_t major, minor;
	amdgpu_device_handle *handle = malloc(sizeof(amdgpu_device_handle));
	if (amdgpu_device_initialize(fd, &major, &minor, handle) == 0)
		// Success
		return handle;
	free(handle);
	return NULL;
}

int tc_amd_get_gpu_sensor_value(void *handle, int *reading, int sensor_type) {
	int sensor_enum = 0;
	uint32_t major, minor;
	switch (sensor_type) {
		case SENSOR_TEMP : sensor_enum = AMDGPU_INFO_SENSOR_GPU_TEMP; break;
		default : return -1;
	}
	amdgpu_device_handle *dev_handle = (amdgpu_device_handle*) handle;
	
	return amdgpu_query_sensor_info(*dev_handle, sensor_enum, sizeof(reading), reading);
}

char *tc_amd_get_gpu_name(void *handle) {	
	return strdup(amdgpu_get_marketing_name(*(amdgpu_device_handle*) handle));
}
