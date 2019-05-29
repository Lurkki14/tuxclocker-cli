#ifndef LIBTUXCLOCKER_AMD_H_
#define LIBTUXCLOCKER_AMD_H_

#include <stdint.h>

#include "libtuxclocker.h"

/* Return the file descriptors for valid AMD GPUs in fds and the amount in len, takes the
   size of the array in size. Returns 1 when size is less than the mount of file descriptors.
*/
int tc_amd_get_gpu_fds(uint8_t *len, int **fds, size_t size);
// Return the device handle for the AMD GPU by file descriptor, NULL on failure
void *tc_amd_get_gpu_handle_by_fd(int fd);

/* Monitoring function. This requires a device handle and a pointer to return the data.
   Returns zero on success like amdgpu functions themselves.	
*/
int tc_amd_get_gpu_sensor_value(void *handle, int *reading, int sensor_type);

// Return 0 on success. Pass the buffer for the string to be copied to and its size
int tc_amd_get_gpu_name(void *handle, size_t buf_len, char (*buf)[]);

#endif
