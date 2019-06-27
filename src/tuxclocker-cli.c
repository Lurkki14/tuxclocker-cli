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

// Flags to be set by arguments
char *tunable_value_flag = NULL;
char *tunable_flag = NULL;
char *gpu_index_flag = NULL;

// Struct for parsing arguments as a tree
typedef struct _opt_node {
	char *name;
	size_t children_len;
       	const struct _opt_node **children;

	// Whether this node sets a flag or not, and the flag pointer
	bool sets_flag;
	char **flag_ptr;

	// Function to be called based on the arguments
	int (*function)();
} opt_node;

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
	// Structure of the argument tree

                const opt_node __list = {"list", 0, NULL, 0, NULL, &print_gpu_names};
                const opt_node *_list = &__list;                                            

                                        const opt_node __tunable_value = {"value", 0, NULL, 1, &tunable_value_flag, &assign_gpu_tunable};
                                        const opt_node *_tunable_value = &__tunable_value;

                                const opt_node *set_tunable_ch[] = {_tunable_value};
                                const opt_node __set_tunable = {"tunable", 1, set_tunable_ch, 1, &tunable_flag, NULL};
                                const opt_node *_set_tunable = &__set_tunable;

                        const opt_node *index_ch[] = {_set_tunable};
                        const opt_node __index = {"index", 1, index_ch, 1, &gpu_index_flag, NULL};
                        const opt_node *_index = &__index;

                const opt_node *set_ch[] = {_index};
                const opt_node __set = {"set", 1, set_ch, 0, NULL, NULL};
                const opt_node *_set = &__set;

        const opt_node *root_ch[] = {_list, _set};
        const opt_node _root = {"root", 2, root_ch, 0, NULL, NULL};
        const opt_node *root = &_root;

        // Find the matcing items for the arguments     
        const opt_node *srch_node = root;
        int arg_idx = 1;
        int (*arg_func)() = NULL;
        while (srch_node != NULL && arg_idx < argc) {
                // Flag for breaking out of the loop when no match was found
                int no_match = 1;
                // Search the children for a matching name
                for (uint8_t i=0; i<srch_node->children_len; i++) {
                        // This node sets a flag eg. GPU index, value, fanmode 
                        if (srch_node->children[i]->sets_flag) {
				// Assign the function if not null
                                if (srch_node->children[i]->function != NULL) {
                                        arg_func = srch_node->children[i]->function;
                                }  
                                *(srch_node->children[i]->flag_ptr) = argv[arg_idx];
                                srch_node = srch_node->children[i];
                                printf("%s\n", argv[arg_idx]);
                                arg_idx++;
                                no_match = 0;
                                break;
                        }   
			
                        if (strcmp(argv[arg_idx], srch_node->children[i]->name) == 0) { 
                                // Assign the function if not null
                                if (srch_node->children[i]->function != NULL) {
                                        arg_func = srch_node->children[i]->function;
                                }   
                                srch_node = srch_node->children[i];
                                printf("%s\n", argv[arg_idx]);
                                arg_idx++;
                                no_match = 0;
                                break;
                        }   
                }   
                if (no_match) {
                        break;
                }   
        }
	// Call the function that was assigned
	if (arg_func != NULL) {
		return arg_func();
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

int print_gpu_names() {
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
	return 0;
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

int assign_gpu_tunable() {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++)
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);

	// Check if the flags are valid
	// Check if index is valid
	// TODO: add an option to apply for all GPUs or a range
	int idx = -1;
	if (!contains_digit(gpu_index_flag)) {
		fprintf(stderr, "Error: not a GPU index: %s\n", gpu_index_flag);
		return 1;
	} else {
		idx = atoi(gpu_index_flag);
	}
	if (gpu_list_len - 1 < idx || idx < 0) {
               fprintf(stderr, "Error: no GPU with index %d\n", idx);
               return 1;
        }

	// Check if tunable is valid
	int tunable_enum = -1;
	for (uint8_t i=0; i<sizeof(tunable_arg_names) / sizeof(char**); i++) {
		if (strcmp(tunable_arg_names[i], tunable_flag) == 0) {
			tunable_enum = i;
			break;
		}
	}
	if (tunable_enum == -1) {
		fprintf(stderr, "Error: no such tunable as %s\n", tunable_flag);
		return 1;
	}

	int target_value = -1;
	// Fan mode is assigned by text
	switch (tunable_enum) {
		case TUNABLE_FAN_MODE:
			// Find the matching enum for the argument
			for (int i=0; i<sizeof(fan_mode_arg_names) / sizeof(char**); i++) {
				if (strcmp(fan_mode_arg_names[i], tunable_value_flag) == 0) {
					target_value = i;
					break;
				}
			}
			break;
		default:
			// For others, check that the argument is only digits
			if (!contains_alpha(tunable_value_flag)) {
				target_value = atoi(tunable_value_flag);
			}
			break;
	}
	// Failed to assign target value
	if (target_value == -1) {
		fprintf(stderr, "Error: invalid value for tunable %s: %s\n", tunable_flag, tunable_value_flag);
		return 1;
	}

	// Call the function
	int retval = 1;
	switch (gpu_list[idx].gpu_type) {
		case AMD: ;
			int (*amd_assign_value)(int, int, const char*, void*) = dlsym(libtc_amd, "tc_amd_assign_value");
			int retval = amd_assign_value(tunable_enum, target_value, gpu_list[idx].hwmon_path, gpu_list[idx].amd_handle);
			if (retval != 0) {
				fprintf(stderr, "Error: failed to assign tunable %s to %s: %s\n", tunable_flag, tunable_value_flag, strerror(errno));
			}
		default:
			break;
	}
	return retval;

	/*int (*amd_assign_value)(int, int, const char*, void*) = dlsym(libtc_amd, "tc_amd_assign_value");
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
	}*/
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
