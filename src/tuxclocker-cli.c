#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/libtuxclocker.h"
#include "amd_functions.h"
#include "tuxclocker-cli.h"

char *sensor_names[16] = {"Temperature", "Fan Speed", "Fan Speed", "Core Clock", "Core Voltage", "Power Draw",
        "Core utilization", "Memory Clock", "Memory Utilization", "Memory Usage"};


// Library handles for different GPU vendors
void *libtc_amd = NULL;
void *libtc_nvidia = NULL;

// Store the different handler structs
gpu_handler *gpu_handler_list = NULL;
uint8_t gpu_handler_list_len = 0;

// Store GPU structs
gpu *gpu_list = NULL;
uint8_t gpu_list_len = 0;

// Remember to handle GPU vendors always in the same order for consistent numbering, 1 = AMD, 2 = Nvidia

int main(int argc, char **argv) {
	// Check for the existence of usable libraries
	libtc_amd = dlopen("libtuxclocker_amd.so", RTLD_NOW);
	libtc_nvidia = dlopen("libtuxclocker_nvidia.so", RTLD_NOW);

	if (libtc_amd == NULL && libtc_nvidia == NULL) {
		fprintf(stderr, "Error: no usable libraries found, exiting...\n");
		return 1;
	}

	// Set the handlers for libraries
	if (libtc_amd != NULL) {
		gpu_handler_list_len++;
		gpu_handler_list = realloc(gpu_handler_list, sizeof(gpu_handler) * gpu_handler_list_len);
		
		gpu_handler_list[gpu_handler_list_len - 1].setup_function = &amd_setup_gpus;
		gpu_handler_list[gpu_handler_list_len - 1].lib_handle = libtc_amd;
	}		

	for (int i=1; i<argc; i++) {
		if (strcmp(argv[i], "--list") == 0) {
			print_gpu_info();
			break;
		}
		if (strcmp(argv[i], "--list-sensors") == 0) {
			print_gpu_sensor_values(0);
			break;
		}
	}
	return 0;
}

void print_gpu_info() {
	for (uint8_t i=0; i<gpu_handler_list_len; i++) {
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);
	}	
	
	// Get the amount of GPUs and print their index and name
	char **gpu_names = malloc(sizeof(char*) * gpu_list_len);
	size_t longest_name_len = 0; // Store the size of the longest GPU name
	char buf[MAX_STRLEN]; // Store GPU names

	uint8_t valid_names_len = 0;
	int retval = 0;
	int (*amd_get_gpu_name)(void*, size_t, char(*)[]) = dlsym(libtc_amd, "tc_amd_get_gpu_name");
	for (uint8_t i=0; i<gpu_list_len; i++) {
		switch (gpu_list[i].gpu_type) {
			case AMD:
		       		retval = amd_get_gpu_name(gpu_list[i].amd_handle, MAX_STRLEN, &buf);
				if (retval != 0)
					continue;
				
				size_t name_len = strlen(buf);
				if (name_len > longest_name_len)
					longest_name_len = name_len;

				valid_names_len++;
				gpu_names[valid_names_len - 1] = strdup(buf);
				break;
			default: continue;
		}
	}

	// Print the GPU names and indices
	printf("%-*s %8s\n", longest_name_len, "GPU", "Index");
	for (uint8_t i=0; i<gpu_list_len; i++) {
		printf("%-*s %8u\n", longest_name_len, gpu_names[i], i);
	}
	// Free the memory
	for (int i=0; i<gpu_list_len; i++)
		free(gpu_names[i]);	

	free(gpu_names);		
}

void print_gpu_sensor_values(int idx) {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++)
	       gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len); 	
	
	int (*amd_get_sensor_value)(void*, int*, int, const char*) = dlsym(libtc_amd, "tc_amd_get_gpu_sensor_value");
	int reading = 0;
	int retval = 0;

	printf("%s\n", gpu_list[0].hwmon_path);

	for (uint8_t i=0; i<gpu_list_len; i++) {
		printf("Sensor readings for GPU %u:\n", i);
		switch (gpu_list[i].gpu_type) {
			case AMD:
				// Try to print a value for all sensor enums
				for (int j=SENSOR_TEMP; j<SENSOR_MEMORY_MB_USAGE + 1; j++) { 
					retval = amd_get_sensor_value(gpu_list[i].amd_handle, &reading, j, gpu_list[i].hwmon_path);
					if (retval == 0)
						printf("\t%s: %d\n", sensor_names[j], reading);
						//printf("\t%s\n", gpu_list[i].hwmon_path);
				}
			default: continue;
		}
	}
}
