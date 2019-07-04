#ifndef TUXCLOCKER_CLI_H_
#define TUXCLOCKER_CLI_H_

#include <stdbool.h>

// Used for some functions that require a size as an argument
#define  MAX_GPUS 64
#define  MAX_STRLEN 128
#define  MAX_STRUCT_SIZE 512

// Struct for initializing all GPUs
typedef struct {
	int gpu_type;
	void *amd_handle;
	int fd;
	char *hwmon_path; // Path for reading AMD sysfs

	// NVML and XNVCtrl handles (XNVCtrl handle is just a Display*, but there's no reason to include its definition here)
	void *nvctrl_handle;
	void *nvml_handle;
} gpu;


typedef struct {
	void *lib_handle;
	int (*setup_function)(void*, gpu**, uint8_t*);
} gpu_handler;

// Print the GPU name and the index the program uses. Always returns 0
int print_gpu_names();

// Print available sensors for GPU by index. Print for all GPU's if idx < 0
int print_gpu_sensor_values();

// Assign a value for a tunable by GPU index
int assign_gpu_tunable();

// Print help
int print_help();

// Print pstate info for AMD GPUs
int print_pstate_info();

// List available tunables and their range.
int list_tunables();

void assign_pstate(int idx, char *pstate_type, int clock, int voltage);

// Functions for checking validity of arguments
bool contains_digit(const char *str);
bool contains_alpha(const char *str);

// Function to check if a GPU by an index exists. Returns the index if idx_char is valid and -1 when not.
int get_gpu_index(uint8_t gpu_count, char *idx_char);

#endif
