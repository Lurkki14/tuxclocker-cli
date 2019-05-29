#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/libtuxclocker.h"
#include "tuxclocker-cli.h"

// Library handles for different GPU vendors
void *libtc_amd = NULL;
void *libtc_nvidia = NULL;

// Remember to handle GPU vendors always in the same order, 1 = AMD, 2 = Nvidia

int main(int argc, char **argv) {
	// Check for the existence of usable libraries
	libtc_amd = dlopen("libtuxclocker_amd.so", RTLD_NOW);
	
	if (libtc_amd == NULL) {
		fprintf(stderr, "Error: no usable libraries found, exiting...\n");
		return 1;
	}

	/*libtc_amd = dlopen("libtuxclocker_amd.so", RTLD_NOW);
	if (libtc_amd != NULL) {
		
		printf("non-null\n");
		int *(*amd_get_gpu_count)(uint8_t*) = dlsym(libtc_amd, "tc_amd_get_gpu_fds");
		uint8_t amount = 0;
		int *entries = amd_get_gpu_count(&amount);
		printf("Found %d AMD GPUs\n", amount);

		printf("File descriptors:\n");
		for (int i=0; i<amount; i++) {
			printf("%d ", entries[i]);
		}
		printf("\n");

		void *(*amd_get_gpu_handle_by_fd)(int) = dlsym(libtc_amd, "tc_amd_get_gpu_handle_by_fd");
		if (amd_get_gpu_handle_by_fd == NULL) printf("Null function\n");
		void *handle = amd_get_gpu_handle_by_fd(entries[0]);
		if (handle == NULL) {
			printf("Null handle\n");
			return 1;
		}

		int (*amd_get_gpu_sensor_value)(void*, int*, int) = dlsym(libtc_amd, "tc_amd_get_gpu_sensor_value");
		int reading = 0;
		int retval = amd_get_gpu_sensor_value(handle, &reading, SENSOR_TEMP);
		if (retval < 0) perror("Error");
		printf("Retval: %d\n", retval);
		

		printf("%d\n", reading);
		
	}*/
	for (int i=1; i<argc; i++) {
		if (strcmp(argv[i], "list") == 0) {
			print_gpu_info();
			break;
		}
	}
	return 0;
}

void print_gpu_info() {
	// Get the amount of GPUs and print their index and name
	char **gpu_names = NULL;
	uint8_t gpu_count = 0;
	size_t longest_name_len = 0; // Store the size of the longest GPU name
	char buf[MAX_STRLEN]; // Store GPU names
	if (libtc_amd != NULL) {
		uint8_t amd_gpu_count = 0;
		
		int (*amd_get_gpu_fds)(uint8_t*, int**, size_t) = dlsym(libtc_amd, "tc_amd_get_gpu_fds");
		int *fds = malloc(sizeof(int) * MAX_GPUS);
		int retval = amd_get_gpu_fds(&amd_gpu_count, &fds, MAX_GPUS);
		if (retval != 0) {
			fprintf(stderr, "Error: failed to get file descriptors for AMD GPUs\n");
			return;
		}
		// Allocate more memory for GPU names
		gpu_names = realloc(gpu_names, sizeof(char*) * gpu_count + amd_gpu_count);

		void *(*amd_get_gpu_handle_by_fd)(int) = dlsym(libtc_amd, "tc_amd_get_gpu_handle_by_fd");
		int (*amd_get_gpu_name)(void*, size_t, char(*)[]) = dlsym(libtc_amd, "tc_amd_get_gpu_name");
		// Get the handles and names		
		for (uint8_t i=0; i<amd_gpu_count; i++) {
			gpu_count++;
			void *handle = amd_get_gpu_handle_by_fd(fds[i]);
			if (handle == NULL)
				continue;

			if (amd_get_gpu_name(handle, MAX_STRLEN, &buf) != 0)
				continue;

			size_t name_len = strlen(buf);
			if (name_len > longest_name_len)
				longest_name_len = name_len;

			gpu_names[gpu_count - 1] = strdup(buf);
			free(handle);						
		}
		free(fds);	
	}	
	// Print the GPU names and indices
	printf("%-*s %8s\n", longest_name_len, "GPU", "Index");
	for (uint8_t i=0; i<gpu_count; i++) {
		printf("%-*s %8u\n", longest_name_len, gpu_names[i], i);
	}
	// Free the memory
	for (int i=0; i<gpu_count; i++)
		free(gpu_names[i]);	

	free(gpu_names);
}	
