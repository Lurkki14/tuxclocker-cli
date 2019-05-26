#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <libdrm/amdgpu.h>

#include "libtuxclocker_amd.h"
int tc_amd_get_gpu_count() {
	const char *dev_dir_name = "/dev/dri";
	int amount = 0;
	// Try to initialize all renderD* devices in /dev/dri
	DIR *dev_dir;
	struct dirent *dev_entry;
	char dev_abs_path[128];

	// Open the directory
	dev_dir = opendir(dev_dir_name);
	if (dev_dir == NULL)
		return -1;		
	else {
		// Try to open the files containing renderD
		int fd = 0;
		uint32_t major, minor;
		while ((dev_entry = readdir(dev_dir)) != NULL) {
			if (strstr(dev_entry->d_name, "renderD") != NULL) {
				sprintf(dev_abs_path, "%s/%s", dev_dir_name, dev_entry->d_name);

				fd = open(dev_abs_path, O_RDONLY);
				if (fd < 1)
					continue;
				// Create a device handle and try to initialize 
				amdgpu_device_handle dev_handle;
				if (amdgpu_device_initialize(fd, &major, &minor, &dev_handle) == 0)
					// Success
					amount++;
			}
		}
	}
	return amount;
}
	
