#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "../lib/libtuxclocker.h"
#include "amd_functions.h"
#include "tuxclocker-cli.h"

// Assign extern variables
char *sensor_names[16] = {"Temperature", "Fan Speed", "Fan Speed", "Core Clock", "Core Voltage", "Power Draw",
        "Core utilization", "Memory Clock", "Memory Utilization", "Memory Usage"};

char *tunable_arg_names[16] = {"fanspeed", "fanmode", "powerlimit", "coreclock", "memclock", "corevoltage", "memvoltage"};

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
	if (argc < 2) {
		printf("No options specified. Run with --help for help.\n");
		return 0;
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
		if (strcmp(argv[i], "--list_sensors") == 0) {
			print_gpu_sensor_values(0);
			break;
		}
		if (strcmp(argv[i], "--help") == 0) {
			print_help();
			break;
		}
		if (strcmp(argv[i], "--set_tunable") == 0) {
			// Read arguments of the format [index, type, value]
			if (i + 3 < argc) {
				// Check if arguments are valid
				if (!contains_alpha(argv[i+1]) && !contains_digit(argv[i+2]) && !contains_alpha(argv[i+3])) {
					// Got valid arguments - check the validity of the type in the function
					assign_gpu_tunable(atoi(argv[i + 1]), argv[i + 2], atoi(argv[i + 3]));
				}
			} else {
				printf("--set_tunable: not enough arguments\n");
			}				
			break;
		}
		if (strcmp(argv[i], "--list_pstate_info") == 0) {
			print_pstate_info();
		}
	}
	return 0;
}

bool contains_digit(const char *str) {
	while (*str) {
		if (isdigit(*str) != 0)
			return true;
		*str++;
	}
	return false;
}

bool contains_alpha(const char *str) {
	while (*str) {
		if (isalpha(*str) != 0)
			return true;
		*str++;
	}
	return false;
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

void print_pstate_info() {
	// Setup GPUs
        for (uint8_t i=0; i<gpu_handler_list_len; i++)
               gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);

 	// Get pstate info
	int (*amd_get_pstate_info)(amd_pstate_info*, const char*) = dlsym(libtc_amd, "tc_amd_get_pstate_info");
    	int retval = 0;

	for (uint8_t i=0; i<gpu_list_len; i++) {
		if (gpu_list[i].gpu_type == AMD) {
			amd_pstate_info info;
			retval = amd_get_pstate_info(&info, gpu_list[i].hwmon_path);
			printf("%d\n", retval);
			if (retval != 0)
				continue;

			// Print the core pstates
			printf("Core pstates for GPU %d:\n", i);
			printf("\t%-4s %-8s %-8s", "Index", "Frequency", "Voltage");
			for (uint8_t j=0; j<info.c_pstate_count; j++) {
				printf("\t%-4u %-8u %-8u", j, info.c_clocks[j], info.c_voltages[j]);
			}
			// Print memory pstates
			printf("Memory pstates for GPU %d:\n", i);
			printf("\t%-4s %-8s %-8s", "Index", "Frequency", "Voltage");
                        for (uint8_t j=0; j<info.m_pstate_count; j++) {  
                                printf("\t%-4u %-8u %-8u", j, info.m_clocks[j], info.m_voltages[j]);
                        }	
		}
	}
}	

void print_gpu_sensor_values(int idx) {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++)
	       gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len); 	
	
	int (*amd_get_sensor_value)(void*, int*, int, const char*) = dlsym(libtc_amd, "tc_amd_get_gpu_sensor_value");
	int reading = 0;
	int retval = 0;

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

void list_tunables(int idx) {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++)
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);

		
}

void assign_gpu_tunable(int idx, char *tunable_name, int target_value) {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++)
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);

	// Check if index is invalid
	if (gpu_list_len - 1 < idx || idx < 0) {
	       printf("--set_tunable: invalid index\n");
	       return;
	}

	int (*amd_assign_value)(int, int, const char*) = dlsym(libtc_amd, "tc_amd_assign_value");
	// Check what index in tunable_arg_names tunable_names matches - it's the enum of tunable_type
	for (int i=0; i<TUNABLE_MEMORY_VOLTAGE + 1; i++) {
		if (strcmp(tunable_arg_names[i], tunable_name) == 0) {
			// Found a matching name, call the function
			int retval = 0;
			switch (gpu_list[idx].gpu_type) {
				case AMD:
					retval = amd_assign_value(i, target_value, gpu_list[idx].hwmon_path);
					if (retval != 0) {
						printf("Error: couldn't assign tunable %s to value %d\n", tunable_arg_names[i], target_value);
		
					}
			}
		}
	}		
}

void print_help() {
	printf("usage: tuxclocker_cli [option]\n");
	printf("Options:\n");
	printf("  --list\n");
	printf("  --list_sensors\n");
	printf("  --set_tunable <index tunable value>\n");
	printf("  --list_pstate_info\n");
	printf("  \tWhere tunable is one of: fanspeed, fanmode, powerlimit, coreclock, memclock, corevoltage, memvoltage\n"); 
}
