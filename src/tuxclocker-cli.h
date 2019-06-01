#ifndef TUXCLOCKER_CLI_H_
#define TUXCLOCKER_CLI_H_

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
} gpu;


typedef struct {
	void *lib_handle;
	int (*setup_function)(void*, gpu**, uint8_t*);
} gpu_handler;

// Print the GPU name and the index the program uses
void print_gpu_info();

// Print available sensors for GPU by index. Print for all GPU's if idx < 0
void print_gpu_sensor_values(int idx);

#endif
