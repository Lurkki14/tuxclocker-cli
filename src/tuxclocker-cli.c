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
#include "nvidia_functions.h"
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
		printf("No options specified. Run with 'help' for help.\n");
		return 0;
	}

	// Set the handlers for libraries
	if (libtc_amd != NULL) {
		gpu_handler_list_len++;
		gpu_handler_list = realloc(gpu_handler_list, sizeof(gpu_handler) * gpu_handler_list_len);
		
		gpu_handler_list[gpu_handler_list_len - 1].setup_function = &amd_setup_gpus;
		gpu_handler_list[gpu_handler_list_len - 1].lib_handle = libtc_amd;
	}
	if (libtc_nvidia != NULL) {
		gpu_handler_list_len++;
                gpu_handler_list = realloc(gpu_handler_list, sizeof(gpu_handler) * gpu_handler_list_len);

                gpu_handler_list[gpu_handler_list_len - 1].setup_function = &nvidia_setup_gpus;
                gpu_handler_list[gpu_handler_list_len - 1].lib_handle = libtc_nvidia;
	}

	// Structure of the argument tree
				const opt_node __list_props = {"props", 0, NULL, 0, NULL, &print_gpu_properties};
				const opt_node *_list_props = &__list_props;
				
				const opt_node __list_pstates = {"pstates", 0, NULL, 0, NULL, &print_pstate_info};
				const opt_node *_list_pstates = &__list_pstates;
	
				const opt_node __list_tunables = {"tunables", 0, NULL, 0, NULL, &list_tunables};
				const opt_node *_list_tunables = &__list_tunables;

				const opt_node __list_sensors = {"sensors", 0, NULL, 0, NULL, &print_gpu_sensor_values};
				const opt_node *_list_sensors = &__list_sensors;

			const opt_node *l_index_ch[] = {_list_sensors, _list_tunables, _list_pstates, _list_props};
                        const opt_node _l_index = {"index", 4, l_index_ch, 1, &gpu_index_flag, NULL};
                        const opt_node *l_index = &_l_index;

		const opt_node *list_ch[] = {l_index};
                const opt_node __list = {"list", 1, list_ch, 0, NULL, &print_gpu_names};
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

		const opt_node __help = {"help", 0, NULL, 0, NULL, &print_help};
		const opt_node *_help = &__help;

        const opt_node *root_ch[] = {_list, _set, _help};
        const opt_node _root = {"root", 3, root_ch, 0, NULL, NULL};
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
	while (*str != '\0') {
		if (isdigit(*str++) != 0)
			return true;
	}
	return false;
}

bool contains_alpha(const char *str) {
	while (*str != '\0') {
		if (isalpha(*str++) != 0)
			return true;
	}
	return false;
}

int get_gpu_index(uint8_t gpu_count, char *idx_char) {
	int idx = -1;
	if (!contains_digit(idx_char)) {
		fprintf(stderr, "Error: not a GPU index: %s\n", idx_char);
		return idx;
	} else {
		idx = atoi(gpu_index_flag);
	}
	if (gpu_list_len - 1 < idx || idx < 0) {
		fprintf(stderr, "Error: no GPU with index %d\n", idx);
		return -1;
	}
	return idx;
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
	size_t name_len = 0;
	int (*amd_get_gpu_name)(void*, size_t, char(*)[]) = dlsym(libtc_amd, "tc_amd_get_gpu_name");
	for (uint8_t i=0; i<gpu_list_len; i++) {
		switch (gpu_list[i].gpu_type) {
			case AMD:
		       		retval = amd_get_gpu_name(gpu_list[i].amd_handle, MAX_STRLEN, &buf);
				if (retval != 0)
					continue;
				
				name_len = strlen(buf);
				if (name_len > longest_name_len)
					longest_name_len = name_len;

				valid_names_len++;
				gpu_names[valid_names_len - 1] = strdup(buf);
				break;
			case NVIDIA: ;
				int (*nv_get_gpu_name)(void*, char (*)[], size_t) = dlsym(libtc_nvidia, "tc_nvidia_get_nvml_gpu_name");
				retval = nv_get_gpu_name(gpu_list[i].nvml_handle, &buf, MAX_STRLEN);
				if (retval != 0) {
					continue;
				}
				name_len = strlen(buf);
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
	for (uint8_t i=0; i<valid_names_len; i++) {
		printf("%-*s %8u\n", (int) longest_name_len, gpu_names[i], i);
	}
	// Free the memory
	for (int i=0; i<valid_names_len; i++)
		free(gpu_names[i]);

	free(gpu_names);		
	return 0;
}

int print_pstate_info() {
	// Setup GPUs
        for (uint8_t i=0; i<gpu_handler_list_len; i++)
               gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);

 	// Get pstate info
	int (*amd_get_pstate_info)(amd_pstate_info*, const char*) = dlsym(libtc_amd, "tc_amd_get_pstate_info");
    	int retval = 0;

	int idx = get_gpu_index(gpu_list_len, gpu_index_flag);
	if (idx < 0) {
		return 1;
	}

	if (gpu_list[idx].gpu_type == AMD) {
		amd_pstate_info info = {0,0,0,0,0,0,0,0,0,0,0,0};
		retval = amd_get_pstate_info(&info, gpu_list[idx].hwmon_path);
		if (retval != 0) {
			return 1;
		}

		// Print the core pstates
		if (info.c_pstate_count > 0) {
			printf("Core pstates for GPU %u:\n", idx);
			printf("\t%-6s %-18s %-18s", "Index", "Frequency (MHz)", "Voltage (mV)\n");
			for (uint8_t j=0; j<info.c_pstate_count; j++) {
				printf("\t%-6u %-18u %-18u\n", j, info.c_clocks[j], info.c_voltages[j]);
			}
		}
		// Print memory pstates
		if (info.m_pstate_count > 0) {
			printf("Memory pstates for GPU %u:\n", idx);
			printf("\t%-6s %-18s %-18s", "Index", "Frequency (MHz)", "Voltage (mV)\n");
			for (uint8_t j=0; j<info.m_pstate_count; j++) {  
				printf("\t%-6u %-18u %-18u\n", j, info.m_clocks[j], info.m_voltages[j]);
			}
		}
		// Print limits
		if (info.min_voltage != 0 && info.max_voltage != 0) {
			printf("Pstate value limits for GPU %u:\n", idx);
			printf("\tVoltage: %u - %u mV\n", info.min_voltage, info.max_voltage);
			printf("\tCore clock: %u - %u MHz\n", info.min_c_clock, info.max_c_clock);
			printf("\tMemory clock: %u - %u MHz\n", info.min_m_clock, info.max_m_clock);
		}
	}
	return 0;
}	

int print_gpu_sensor_values() {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++)
	       gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len); 	
	
	// Check if index is valid
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

	int retval = 0;
	int reading = 0;
	printf("Sensor readings for GPU %d:\n", idx);
	switch (gpu_list[idx].gpu_type) {
		case AMD: ;
			int (*amd_get_sensor)(void*, int*, int, int, const char*) = dlsym(libtc_amd, "tc_amd_get_gpu_sensor_value");
			// Try to print a value for all sensor enums
			for (int i=0; i<sizeof(sensor_names) / sizeof(char**); i++) {
				retval = amd_get_sensor(gpu_list[idx].amd_handle, &reading, i, gpu_list[idx].fd, gpu_list[idx].hwmon_path);
				if (retval == 0) {
					printf("\t%s: %d %s\n", sensor_names[i], reading, sensor_units[i]);
				}
			}
		case NVIDIA: ;
			int (*nvidia_get_sensor)(void*, void*, sensor_info*, int, int) = dlsym(libtc_nvidia, "tc_nvidia_get_sensor_value");
			sensor_info info;
			for (uint8_t i=0; i<sizeof(sensor_names) / sizeof(char**); i++) {
				retval = nvidia_get_sensor(gpu_list[idx].nvml_handle, gpu_list[idx].nvctrl_handle, &info, i, gpu_list[idx].nvidia_index);
				if (retval != 0) {
					continue;
				}
				// Display the correct data type
				switch (info.sensor_data_type) {
					case SENSOR_TYPE_UINT:
						printf("\t%s: %u %s\n", sensor_names[i], info.readings.u_reading, sensor_units[i]);
						break;
					case SENSOR_TYPE_DOUBLE:
						printf("\t%s: %f %s\n", sensor_names[i], info.readings.d_reading, sensor_units[i]);
						break;
					default:
						break;
				}
			}
		default:
			return 1;
	}

	return 0;
}

int print_gpu_properties() {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++) {
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);
	}

	int idx = get_gpu_index(gpu_list_len, gpu_index_flag);
	if (idx < 0) {
		// Index argument was invalid
		return 1;
	}

	int retval = 0;
	printf("Properties for GPU %d:\n", idx);
	switch (gpu_list[idx].gpu_type) {
		case NVIDIA: ;
			int (*nv_get_property)(void*, void*, sensor_info*, int, int) = dlsym(libtc_nvidia, "tc_nvidia_get_property_value");
			sensor_info info;
			// Try to print a value for all properties
			for (uint8_t i=0; i<sizeof(gpu_properties) / sizeof(char**); i++) {
				retval = nv_get_property(gpu_list[idx].nvml_handle, gpu_list[idx].nvctrl_handle, &info, i, gpu_list[idx].nvidia_index);
				if (retval != 0) {
					continue;
				}
				// Display the correct data type
				switch (info.sensor_data_type) {
					case SENSOR_TYPE_UINT:
						printf("\t%s: %u %s\n", gpu_properties[i], info.readings.u_reading, gpu_property_units[i]);
						break;
					default:
						break;
				}
			}
	}

	return 0;
}

int list_tunables() {
	// Setup GPUs
	for (uint8_t i=0; i<gpu_handler_list_len; i++) {
		gpu_handler_list[i].setup_function(gpu_handler_list[i].lib_handle, &gpu_list, &gpu_list_len);
	}
	int idx = get_gpu_index(gpu_list_len, gpu_index_flag);
	if (idx < 0) {
		// The error message is printed in the function
		return 1;
	}

	int cur_val_retval = 0;
	int retval = 0;
	printf("Available tunables for GPU %u:\n", idx);
	switch (gpu_list[idx].gpu_type) {
		case AMD: ; 
			// Check what tunables we get a range successfully for
			tunable_valid_range range;
			for (int j=0; j<sizeof(tunable_names) / sizeof(char**); j++) {
				retval = amd_get_tunable_range(&range, j, gpu_list[idx].hwmon_path,
						gpu_handler_list[idx].lib_handle, gpu_list[idx].amd_handle);
				if (retval == 0) {
					printf("\t%s: range %u - %u %s, Value type: %s\n", tunable_names[j], range.min, range.max, tunable_units[j], tunable_value_type_names[range.tunable_value_type]);
				}
			}
		case NVIDIA: {
			tunable_valid_range range;
			int tunable_value = 0;
			int (*nvidia_get_range)(void*, void*, tunable_valid_range*, int, int, int) = dlsym(libtc_nvidia, "tc_nvidia_get_tunable_range");
			int (*nvidia_get_value)(void*, void*, int*, int, int, int) = dlsym(libtc_nvidia, "tc_nvidia_get_tunable_value");
			for (uint8_t i=0; i<sizeof(tunable_names) / sizeof(char**); i++) {
					retval = nvidia_get_range(gpu_list[idx].nvml_handle, gpu_list[idx].nvctrl_handle, &range, i, gpu_list[idx].nvidia_index, gpu_list[idx].nvidia_pstate_count - 1);
					cur_val_retval = nvidia_get_value(gpu_list[idx].nvml_handle, gpu_list[idx].nvctrl_handle, &tunable_value, i, gpu_list[idx].nvidia_index, gpu_list[idx].nvidia_pstate_count - 1);
					if (retval == 0) {
						printf("\t%s: range %d - %d %s, Value type: %s", tunable_names[i], range.min, range.max, tunable_units[i], tunable_value_type_names[range.tunable_value_type]);
					}
					// Also print the current value if querying it was successful
					if (cur_val_retval == 0) {
						printf(", Current Value: %d %s", tunable_value, tunable_units[i]);
					}
					if (retval == 0 || cur_val_retval == 0) {
						// Print a newline if some info was printed
						printf("\n");
					}
			}
		}
		default: return 1;
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
			break;
		case NVIDIA: ;
			int (*nv_assign_value)(void*, void*, int, int, int, int) = dlsym(libtc_nvidia, "tc_nvidia_assign_value");
			retval = nv_assign_value(gpu_list[idx].nvml_handle, gpu_list[idx].nvctrl_handle, tunable_enum, target_value, gpu_list[idx].nvidia_index, gpu_list[idx].nvidia_pstate_count - 1);
			if (retval != 0) {
				char *error_string = "";
				// Get the error message from the right library
				switch (tunable_enum) {
					case TUNABLE_POWER_LIMIT: ;
						char *(*nvml_get_error)(int) = dlsym(libtc_nvidia, "tc_nvidia_nvml_error_string_from_retval");
						error_string = nvml_get_error(retval);
						break;
					default:
						break;
				}
				fprintf(stderr, "Error: failed to assign tunable %s to %s: %s\n", tunable_flag, tunable_value_flag, error_string);
			}
			break;
		default:
			break;
	}
	return retval;
}

int print_help() {
	static const char *help_message = "usage: tuxclocker-cli [options]\n"
					"Options are read as a hierarchical tree, so for example to list sensor\n"
					"readings for GPU 0: do 'tuxclocker-cli list 0 sensors'\n"
					"Options:\n"
					"  list\n"
					"    <index>\n"
					"      sensors\n"
					"      tunables\n"
					"      pstates\n"
					"  set\n"
					"    <index>\n"
					"      <tunable>\n"
					"        Where tunable is one of: fanspeed, fanmode, powerlimit, coreclock, memclock, corevoltage, memvoltage\n"
					"          <value>\n"
					"            Values for fanmode: auto, manual\n"
					"  help\n";

	printf("%s", help_message);
	return 0;
}
