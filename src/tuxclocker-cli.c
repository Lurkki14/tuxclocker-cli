#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

#include "../lib/libtuxclocker.h"
#include "amd_functions.h"
#include "tuxclocker-cli.h"

extern int errno;

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
		if (strcmp(argv[i], "--set_pstate") == 0) {
			// Check if arguments are [index, pstate_type, clock, voltage]
			if (i + 4 < argc) {
				if (!contains_alpha(argv[i+1]) && !contains_digit(argv[i+2]) && !contains_alpha(argv[i+3]) && !contains_alpha(argv[i+4])) {
					assign_pstate(atoi(argv[i+1]), argv[i+2], atoi(argv[i+3]), atoi(argv[i+4]));
					break;
				}
			} else {
				printf("--set_pstate: not enough arguments\n");
			}
		}

		if (strcmp(argv[i], "--set_tunable") == 0) {
			// Read arguments of the format [index, type, value]
			if (i + 3 < argc) {
				// Check if arguments are valid
				if (!contains_alpha(argv[i+1]) && !contains_digit(argv[i+2])) {
					// Got valid arguments - check the validity of the type in the function
					assign_gpu_tunable(atoi(argv[i + 1]), argv[i + 2], argv[i + 3]);
				}
			} else {
				printf("--set_tunable: not enough arguments\n");
			}				
			break;
		}
		if (strcmp(argv[i], "--list_pstate_info") == 0) {
			print_pstate_info();
		}
		if (strcmp(argv[i], "--list_tunables") == 0) {
			list_tunables(0);
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
	printf("%-*s %8s\n", (int) longest_name_len, "GPU", "Index");
	for (uint8_t i=0; i<gpu_list_len; i++) {
		printf("%-*s %8u\n", (int) longest_name_len, gpu_names[i], i);
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
			amd_pstate_info info = {0,0,0,0,0,0,0,0,0,0,0,0};
			retval = amd_get_pstate_info(&info, gpu_list[i].hwmon_path);
			if (retval != 0)
				continue;

			// Print the core pstates
			if (info.c_pstate_count > 0) {
				printf("Core pstates for GPU %u:\n", i);
				printf("\t%-6s %-18s %-18s", "Index", "Frequency (MHz)", "Voltage (mV)\n");
				for (uint8_t j=0; j<info.c_pstate_count; j++) {
					printf("\t%-6u %-18u %-18u\n", j, info.c_clocks[j], info.c_voltages[j]);
				}
			}
			// Print memory pstates
			if (info.m_pstate_count > 0) {
				printf("Memory pstates for GPU %u:\n", i);
				printf("\t%-6s %-18s %-18s", "Index", "Frequency (MHz)", "Voltage (mV)\n");
				for (uint8_t j=0; j<info.m_pstate_count; j++) {  
					printf("\t%-6u %-18u %-18u\n", j, info.m_clocks[j], info.m_voltages[j]);
				}
			}
			// Print limits
			if (info.min_voltage != 0 && info.max_voltage != 0) {
				printf("Pstate value limits for GPU %u:\n", i);
				printf("\tVoltage: %u - %u mV\n", info.min_voltage, info.max_voltage);
				printf("\tCore clock: %u - %u MHz\n", info.min_c_clock, info.max_c_clock);
				printf("\tMemory clock: %u - %u MHz\n", info.min_m_clock, info.max_m_clock);
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
						printf("\t%s: %d %s\n", sensor_names[j], reading, sensor_units[j]);
						//printf("\t%s\n", gpu_list[i].hwmon_path);
				}
			default: continue;
		}
	}
}

void list_tunables(int idx) {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++) {
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);
	}
	int retval = 0;
	for (uint8_t i=0; i<gpu_list_len; i++) {
		printf("Available tunables for GPU %u:\n", i);
		switch (gpu_list[i].gpu_type) {
			case AMD: ; 
				// Check what tunables we get a range successfully for
				tunable_valid_range range;
				for (int j=TUNABLE_FAN_SPEED_PERCENTAGE; j<TUNABLE_MEMORY_VOLTAGE + 1; j++) {
					retval = amd_get_tunable_range(&range, j, gpu_list[i].hwmon_path,
							gpu_handler_list[i].lib_handle, gpu_list[i].amd_handle);
					if (retval == 0) {
						printf("\t%s: range %u - %u %s, Value type: %s\n", tunable_names[j], range.min, range.max, tunable_units[j], tunable_value_type_names[range.tunable_value_type]);
					}
				}
			default: continue;
		}
	}	
}

void assign_pstate(int idx, char *pstate_type, int clock, int voltage) {
	// Setup GPUs               
        for (uint8_t i=0; i<gpu_handler_list_len; i++) {
                gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);
	}
	// Check the pstate_type
	for (int i=0; i<sizeof(amd_pstate_type_args) / sizeof(char**); i++) {
		if (strcmp(pstate_type, amd_pstate_type_args[i]) == 0) {
			// Call the function
			int (*assign_pstate)(int, uint8_t, uint32_t, uint32_t, const char*) = dlsym(libtc_amd, "tc_amd_assign_pstate");
			if (assign_pstate == NULL) {
				fprintf(stderr, "Error: couldn't load library function 'tc_amd_assign_pstate'");
				return;
			}
			int retval = assign_pstate(i, (uint8_t) idx, (uint32_t) clock, (uint32_t) voltage, gpu_list[idx].hwmon_path);
			if (retval != 0) {
				fprintf(stderr, "Error: couldn't assign %s pstate: %s\n", pstate_type, strerror(errno));
			}
			return;
		}
	}
	fprintf(stderr, "Error: no such pstate type as %s\n", pstate_type);
}

void assign_gpu_tunable(int idx, char *tunable_name, char *target_value) {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++)
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);

	// Check if index is invalid
	if (gpu_list_len - 1 < idx || idx < 0) {
	       printf("--set_tunable: invalid index\n");
	       return;
	}

	int (*amd_assign_value)(int, int, const char*, void*) = dlsym(libtc_amd, "tc_amd_assign_value");
	// Check what index in tunable_arg_names tunable_names matches - it's the enum of tunable_type
	for (int i=0; i<TUNABLE_MEMORY_VOLTAGE + 1; i++) {
		if (strcmp(tunable_arg_names[i], tunable_name) == 0) {
			// Found a matching name, call the function
			int retval = 0;
			switch (gpu_list[idx].gpu_type) {
				case AMD:
					// Fan mode is assigned by text
					switch (i) {
						case TUNABLE_FAN_MODE:
							// Check what enum target_value matches
							for (int j=0; j<sizeof(fan_mode_arg_names) / sizeof(char**); j++) {
									if (strcmp(fan_mode_arg_names[j], target_value) == 0) {
										retval = amd_assign_value(i, j, gpu_list[idx].hwmon_path, gpu_list[idx].amd_handle);
										if (retval != 0) {
											printf("Error: couldn't assign tunable %s to value %s: %s\n", tunable_arg_names[i], target_value, strerror(errno));
										}
										return;
									}
							}
							printf("Error: no such fanmode as %s\n", target_value);
						default:
							retval = amd_assign_value(i, atoi(target_value), gpu_list[idx].hwmon_path, gpu_list[idx].amd_handle);
							if (retval != 0) {
								printf("Error: couldn't assign tunable %s to value %d: %s\n", tunable_arg_names[i], atoi(target_value), strerror(errno));
							}
							return;
					}
				default:
					continue;
			}
		}
	}
	// tunable_name didn't match anything in tunable_arg_names
	printf("Error: no such tunable as %s\n", tunable_name);
}

void print_help() {
	static const char *help_message = "usage: tuxclocker-cli [option]\n"
					"Options:\n"
					"  --list\n"
					"  --list_sensors\n"
					"  --list_tunables\n"
					"  --set_tunable <index tunable value>\n"
					"  \tWhere tunable is one of: fanspeed, fanmode, powerlimit, coreclock, memclock, corevoltage, memvoltage\n"
					"  \tValues for fanmode: auto, manual\n"
					"  --set_pstate <index type clock voltage>\n"
					"  \tWhere type is one of: core, mem\n"
					"  --list_pstate_info\n";

	printf("%s", help_message);
}
