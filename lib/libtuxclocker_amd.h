#ifndef LIBTUXCLOCKER_AMD_H_
#define LIBTUXCLOCKER_AMD_H_

#include <stdint.h>

#include "libtuxclocker.h"

/* Return the file descriptors for valid AMD GPUs in fds and the amount in len, takes the
   size of the array in size. Returns 1 when size is less than the mount of file descriptors.
*/
int tc_amd_get_gpu_fds(uint8_t *len, int **fds, size_t size);
// Return 0 on success. fd = file descriptor for GPU, size = size of handle, handle = pointer to write data to
int tc_amd_get_gpu_handle_by_fd(int fd, size_t size, void *handle);

/* Monitoring function. This requires a device handle and a pointer to return the data.
   Returns zero on success like amdgpu functions themselves.	
*/
int tc_amd_get_gpu_sensor_value(void *handle, int *reading, int sensor_type, const char *hwmon_dir_name);

// Return 0 on success. Pass the buffer for the string to be copied to and its size
int tc_amd_get_gpu_name(void *handle, size_t buf_len, char (*buf)[]);

// Return 0 on success. Pass the pointer for the paths to be copied to and its size and max size for each string
int tc_amd_get_hwmon_paths(char ***hwmon_paths, size_t arr_len, size_t str_len);

// Get file descriptors and hwmon paths for all AMD GPUs. Remember to free the memory
int tc_amd_get_fs_info_all(char ***hwmon_paths, int **fds, uint8_t *gpu_count);

#endif
