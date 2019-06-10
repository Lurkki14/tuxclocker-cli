#ifndef LIBTUXCLOCKER_H_
#define LIBTUXCLOCKER_H_

// Enums for types of sensor reading
enum sensor_type {SENSOR_TEMP, SENSOR_FAN_PERCENTAGE, SENSOR_FAN_RPM, SENSOR_CORE_CLOCK, SENSOR_CORE_VOLTAGE, 
	SENSOR_POWER_DRAW, SENSOR_CORE_UTILIZATION, SENSOR_MEMORY_CLOCK, SENSOR_MEMORY_UTILIZATION,
	SENSOR_MEMORY_MB_USAGE};

// Enums for modifying GPU parameters
enum tunable_type {TUNABLE_FAN_SPEED_PERCENTAGE, TUNABLE_FAN_MODE, TUNABLE_POWER_LIMIT, TUNABLE_CORE_CLOCK, TUNABLE_MEMORY_CLOCK,
	TUNABLE_CORE_VOLTAGE, TUNABLE_MEMORY_VOLTAGE};

enum tunable_value_type {TUNABLE_ABSOLUTE, TUNABLE_OFFSET};

// Sensor names for displaying
extern char *sensor_names[16];

// Tunable names to be taken in as arguments
extern char *tunable_arg_names[16];

enum gpu_type {AMD, NVIDIA};

// Struct for AMD pre-Vega VII pstate info
typedef struct {
	// c stands for core, m for memory
	// Assume no GPU has more than 10 pstates
	uint32_t m_voltages[10];
	uint32_t m_clocks[10];
	uint8_t m_pstate_count;

        uint32_t c_voltages[10];
        uint32_t c_clocks[10];
        uint8_t c_pstate_count;
        
        uint32_t min_m_clock;
        uint32_t min_c_clock;
        uint32_t min_voltage;
} amd_pstate_info;

#endif
