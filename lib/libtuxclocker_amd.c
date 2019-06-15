#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>

#include "libtuxclocker_amd.h"
#include "libtuxclocker.h"

int tc_amd_get_gpu_handle_by_fd(int fd, size_t size, void *handle) {
	if (sizeof(amdgpu_device_handle) > size)
		// Pointer not big enough
		return 1;

	uint32_t major, minor;
	if (amdgpu_device_initialize(fd, &major, &minor, (amdgpu_device_handle*) handle) == 0)
		// Success
		return 0;
	
	return 1;
}

int tc_amd_get_fs_info_all(char ***hwmon_paths, int **fds, uint8_t *gpu_count) {
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
		return 1;		

	// Try to open the files containing renderD
	int fd = 0;
	uint8_t amount = 0;
	uint32_t major, minor;
	char **paths = NULL;
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
				// Find the hwmon folder for the renderD device
				char hwmon_dir_name[128];
				snprintf(hwmon_dir_name, 128, "/sys/class/drm/%s/device/hwmon", dev_entry->d_name);

				DIR *hwmon_dir;
				struct dirent *hwmon_entry;
				
				// Open the folder
				hwmon_dir = opendir(hwmon_dir_name);
				if (hwmon_dir == NULL)
					continue;

				// Find the entry containing 'hwmon'
				while ((hwmon_entry = readdir(hwmon_dir)) != NULL) {
					if (strstr(hwmon_entry->d_name, "hwmon") != NULL) {
						// Success
						char hwmon_path[128];
						amount++;
						snprintf(hwmon_path, 128, "%s/%s", hwmon_dir_name, hwmon_entry->d_name);
						paths = realloc(paths, sizeof(char*) * amount);
						paths[amount - 1] = strdup(hwmon_path);

						fd_arr = realloc(fd_arr, sizeof(int) * amount);
						fd_arr[amount - 1] = fd;
					}
				}
				closedir(hwmon_dir);
			}
		}
	}
	// Assign the results to return pointers
	*hwmon_paths = malloc(sizeof(char**) * amount);
	*fds = malloc(sizeof(int) * amount);
	for (uint8_t i=0; i<amount; i++) {
		(*hwmon_paths)[i] = paths[i];
		(*fds)[i] = fd_arr[i];
	}

	closedir(dev_dir);	
	*gpu_count = amount;
	return 0;
}

bool contains_digit(const char *string) {
	while (*string != '\0') {
		if (isdigit(*string) != 0) {
			return true;
		}
		*string++;
	}
	return false;
}

int tc_amd_get_pstate_info(amd_pstate_info *info, const char *hwmon_dir_name) {
	// Change directory to hwmon directory
	if (chdir(hwmon_dir_name) != 0) {
		// Couldn't open folder
		return 1;
	}

	// Go up two directories where pp_od_clk_voltage is located
	if (chdir("../..") != 0) {
		// Failure
		return 1;
	}
	// Open the pp_od_clk_voltage file for reading
	FILE *pstate_file = fopen("pp_od_clk_voltage", "r");
	//FILE *pstate_file = fopen("/home/jussi/Documents/fakepstates", "r");
	if (pstate_file == NULL) {
		// Couldn't open file for reading
		return 1;
	}
	parse_file(info, "", pstate_file, -1);
	return 0;
}

void pstate_info_from_line(const char *line, amd_pstate_info *info, int section_enum) {
        // We expect a string of the format <index, clock, voltage>
        // Get the first space delimited string
        char *n_line = strdup(line);
        char *token = strtok(n_line, " ");

        int i = 0;
        int index = 0;
        uint32_t clock = 0;
        uint32_t voltage = 0;
        while (token != NULL) {
                // Check the validity of the token
                if (!contains_digit(token)) {
                        // The token was unexpected
                        return;
                }

                switch (i) {
                        case 0 : index = atoi(token); break;
                        case 1 : clock = atoi(token); break;
                        case 2 : voltage = atoi(token); break;
                        default : return;
                }
                token = strtok(NULL, " ");
                i++;
        }
	// Successfully parsed expected data
	// Write the values to the correct array
	switch (section_enum) {
		case OD_SCLK :
			info->c_voltages[index] = voltage;
			info->c_clocks[index] = clock;
			info->c_pstate_count = index + 1;
			break;
		case OD_MCLK :
			info->m_voltages[index] = voltage;
			info->m_clocks[index] = clock;
			info->m_pstate_count = index + 1;
			break;
	}
	free(n_line);
}	

void pstate_limit_info_from_line(const char *line, amd_pstate_info *info) {
	// Check what range this line contains
	int range_enum = 0;
        for (uint8_t i=0; i<sizeof(od_range_sections) / sizeof(char**); i++) {
                if (strstr(line, od_range_sections[i]) != NULL) {
                        range_enum = i;
                        break;
                }
        }
        uint32_t max = 0;
        uint32_t min = 0;
        // Get the first token
        char *n_line = strdup(line);
        char *token = strtok(n_line, " ");

        int i = 0;
        while (token != NULL) {
                // Check the validity of the token
                if (!contains_digit(token)) {
                        // Get the next token
                        token = strtok(NULL, " ");
                        continue;
                }
                // The first valid token is the minimum value
                switch (i) {
                        case 0 : min = atoi(token);
                                 break;
                        case 1 : max = atoi(token);
                                 break;
                        default : break;
                }
                // Get the next token
                token = strtok(NULL, " ");
                i++;
        }
        if (max == 0 && min == 0) {
                // Couldn't update the values
                return;
        }
        // Write the values to correct member
        switch (range_enum) {
                case MCLK :
                        info->min_m_clock = min;
                        info->max_m_clock = max;
                        break;
                case SCLK :
                        info->min_c_clock = min;
                        info->max_c_clock = max;
                        break;
                case VDDC :
                        info->min_voltage = min;
                        info->max_voltage = max;
                        break;
                default :
                        break;
        }
        free(n_line);
}

void parse_file(amd_pstate_info *info, const char *section_string, FILE *pstate_file, int section_enum) {
	char *buf = NULL;
	size_t str_len = 0;

	// Get info from this line if it contains any
	switch (section_enum) {
		default : break;
	}
	
	while (getline(&buf, &str_len, pstate_file) != -1) {
		// Check if this is a start of a section
		for (int i=0; i<sizeof(pstate_sections) / sizeof(char**); i++) {
			if (strstr(buf, pstate_sections[i]) != NULL) {
				// New section starts
				parse_file(info, buf, pstate_file, i);
			}
		}
		// Extract data from this line
		switch (section_enum) {
			case OD_SCLK : pstate_info_from_line(buf, info, OD_SCLK);
				       break;
			case OD_MCLK : pstate_info_from_line(buf, info, OD_MCLK);
				       break;
			case OD_RANGE : pstate_limit_info_from_line(buf, info);
					break;
			default : break;
		}
	}
}	

int tc_amd_assign_value(int tunable_enum, int target_value, const char *hwmon_dir_name) {
	// Change directory to hwmon directory
	int retval = chdir(hwmon_dir_name);
	if (retval != 0)
		return 1;
	
	char hwmon_file_name[64];
	// Get the file name to write to
	switch (tunable_enum) {
		case TUNABLE_FAN_SPEED_PERCENTAGE:
				snprintf(hwmon_file_name, 64, "pwm1");
				// Multiply the percentage by 2.55 to get the pwm value
				target_value = (int) round(target_value * 2.55);
				break;
		case TUNABLE_POWER_LIMIT:
				snprintf(hwmon_file_name, 64, "power1_cap");
				// Multiply by million to get the power limit in microwatts
				target_value = (int) round(target_value * 1000000);
				break;
	}
	// Open the file descriptor for writing
	int fd = open(hwmon_file_name, O_WRONLY);
	if (fd < 1) {
		// Couldn't get file descriptor for writing
		return 1;
	}
	// Write the value to a buffer
	char val_buf[16];
	snprintf(val_buf, 16, "%d", target_value);

	// Write the value to the file
	retval = write(fd, val_buf, strlen(val_buf));
	if (retval < 1) {
		// Couldn't write the value
		return 1;
	}
	return 0;
}

int tc_amd_get_tunable_range(int tunable_enum, const char *hwmon_dir_name, tunable_valid_range *range) {
        switch (tunable_enum) {
		case TUNABLE_POWER_LIMIT : goto power_limit_from_files;
					   break;
		default : return 1;
	}
	
power_limit_from_files: ;

	// Change directory to hwmon directory
        int retval = chdir(hwmon_dir_name);
        if (retval != 0) {
                return 1;
	}
	int min = 0, max = 0;
	// Open the power1_cap_min file
	char buf[32];
	FILE *pcap_min_file = fopen("power1_cap_min", "r");
	if (pcap_min_file == NULL) {
		// Couldn't open file
		return 1;
	}
	fscanf(pcap_min_file, "%s", buf);
	fclose(pcap_min_file);
	// Check if buffer contains a digit
	if (!contains_digit(buf)) {
		return 1;
	}
	min = atoi(buf);

	// Open the power1_cap_max file
	FILE *pcap_max_file = fopen("power1_cap_max", "r");
	if (pcap_max_file == NULL) {
		return 1;
	}
	fscanf(pcap_max_file, "%s", buf);
	fclose(pcap_max_file);
	if (!contains_digit(buf)) {
		return 1;
	}
	max = atoi(buf);
	range->min = min;
	range->max - max;
	range->tunable_value_type = TUNABLE_ABSOLUTE;
	return 0;
}	

int tc_amd_get_gpu_sensor_value(void *handle, int *reading, int sensor_type, const char *hwmon_dir_name) {
	int sensor_enum = 0;
	int retval = 0;
	uint32_t major, minor;
	switch (sensor_type) {
		case SENSOR_TEMP : sensor_enum = AMDGPU_INFO_SENSOR_GPU_TEMP; break;
		case SENSOR_CORE_CLOCK : sensor_enum = AMDGPU_INFO_SENSOR_GFX_SCLK; break;			     
		case SENSOR_MEMORY_CLOCK : sensor_enum = AMDGPU_INFO_SENSOR_GFX_MCLK; break;
		case SENSOR_CORE_VOLTAGE : sensor_enum = AMDGPU_INFO_SENSOR_VDDGFX; break;
		case SENSOR_CORE_UTILIZATION : sensor_enum = AMDGPU_INFO_SENSOR_GPU_LOAD; break;
		case SENSOR_POWER_DRAW : sensor_enum = AMDGPU_INFO_SENSOR_GPU_AVG_POWER; break;
		case SENSOR_FAN_PERCENTAGE : goto sensor_file;			 
		default : return 1;
	}
	amdgpu_device_handle *dev_handle = (amdgpu_device_handle*) handle;
	
	retval = amdgpu_query_sensor_info(*dev_handle, sensor_enum, sizeof(reading), reading);
	// Change some reading values to be in line with the units defined in libtuxclocker.h
	switch (sensor_type) {
		default : return retval;
		case SENSOR_TEMP : *reading /= 1000; return retval;
	}
// Label for sensor enums that are read from a file	
sensor_file:
	// Change directory to the hwmon directory
	retval = chdir(hwmon_dir_name);
	if (retval != 0) {
		return 1;		
	}

	// Read the file
	/*FILE *s_file;
	switch (sensor_enum) {
		case SENSOR_FAN_PERCENTAGE :
			s_file = fopen("pwm1", "r");
			if (s_file == NULL) {
				return 1;	
			}

			fscanf(s_file, "%d", reading);
			*reading = (int) *reading / 2.55;
			return 0;
		default : return 1;
	}*/
	FILE *file = fopen("pwm1", "r");
	fscanf(file, "%d", reading);
	*reading = *reading /= 2.55;
	return 0;
}

int tc_amd_get_gpu_name(void *handle, size_t buf_len, char (*buf)[]) {	
	const char *gpu_name = amdgpu_get_marketing_name(*(amdgpu_device_handle*) handle);
	size_t name_len = strlen(gpu_name);
	// buf_len needs to be 1 byte larger due to null terminator 
	if (name_len > buf_len + 1)
		// String to copy to is too small
		return 1;
	
	// Copy the name to the buffer
	snprintf(*buf, name_len + 1, "%s", gpu_name);
	return 0;
}
