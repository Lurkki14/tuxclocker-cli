#ifndef LIBTUXCLOCKER_H_
#define LIBTUXCLOCKER_H_

// Enums for types of sensor reading
enum sensor_type {SENSOR_TEMP, SENSOR_FAN_PERCENTAGE, SENSOR_FAN_RPM, SENSOR_CORE_CLOCK, SENSOR_CORE_VOLTAGE, 
	SENSOR_POWER_DRAW, SENSOR_CORE_UTILIZATION, SENSOR_MEMORY_CLOCK, SENSOR_MEMORY_UTILIZATION,
	SENSOR_MEMORY_MB_USAGE, SENSOR_PCIE_UTILIZATION};

// Enums for querying various other GPU properties eg. total VRAM
enum gpu_property {PROPERTY_UUID, PROPERTY_TOTAL_VRAM, PROPERTY_THROTTLE_TEMP, PROPERTY_SHUTDOWN_TEMP, PROPERTY_GPU_CORE_COUNT, PROPERTY_MEM_BUS_WIDTH, PROPERTY_PCIE_MAX_LINK_SPEED, PROPERTY_PCIE_GEN, PROPERTY_PCIE_LINK_WIDTH, PROPERTY_PCIE_CUR_LINK_SPEED};
static char *const gpu_properties[] = {"UUID", "Total VRAM", "Throttle Temperature", "Shutdown Temperature", "GPU Core Count", "Memory Bus Width", "Max PCIe Link Speed", "PCIe Generation", "PCIe Link Width", "Current PCIe Link Speed"};
static char *const gpu_property_units[] =  {"", "MB", "°C", "°C", "", "Bits", "GT/s", "", "Lanes", "GT/s"};

// Enums for modifying GPU parameters
enum tunable_type {TUNABLE_FAN_SPEED_PERCENTAGE, TUNABLE_FAN_MODE, TUNABLE_POWER_LIMIT, TUNABLE_CORE_CLOCK, TUNABLE_MEMORY_CLOCK,
	TUNABLE_CORE_VOLTAGE, TUNABLE_MEMORY_VOLTAGE};

static char *const tunable_names[] = {"Fan Speed", "Fan Mode", "Power Limit", "Core Clock", "Memory Clock", "Core Voltage", "Memory Voltage"};
static char *const tunable_units[] = {"%", "", "W", "MHz", "MHz", "mV", "mV"};

// Enum for seeing the value type
enum tunable_value_type {TUNABLE_ABSOLUTE, TUNABLE_OFFSET};
static char *const tunable_value_type_names[] = {"Absolute", "Offset"};

// Enum for fan modes
enum fan_mode {FAN_MODE_AUTO, FAN_MODE_MANUAL};
static char *const fan_mode_arg_names[] = {"auto", "manual"};

// Sensor names for displaying values
static char *const sensor_names[] = {"Temperature", "Fan Speed", "Fan Speed", "Core Clock", "Core Voltage", "Power Draw",
        "Core Utilization", "Memory Clock", "Memory Utilization", "Memory Usage", "PCIe Utilization"};
// Units for displaying values
static char *const sensor_units[] = {"°C", "%", "RPM", "MHz", "mV", "W", "%", "MHz", "%", "MB", "%"};

// Tunable names to be taken in as arguments
static char *const tunable_arg_names[] = {"fanspeed", "fanmode", "powerlimit", "coreclock", "memclock", "corevoltage", "memvoltage"};
enum gpu_type {AMD, NVIDIA};

// Pstate types for pre-Vega VII GPUs
enum amd_pstate_type {PSTATE_CORE, PSTATE_MEMORY};
static char *const amd_pstate_type_args[] = {"core", "mem"};

// Data types for getting sensor readings with different data types
enum sensor_data_type {SENSOR_TYPE_UINT, SENSOR_TYPE_DOUBLE, SENSOR_TYPE_STRING};

union sensor_readings {
	double d_reading;
	uint32_t u_reading;
	char *s_reading;
};

typedef struct {
	int sensor_data_type;
	union sensor_readings readings;
} sensor_info;

// Used for getting the limits for a tunable
typedef struct {
	int min;
	int max;

	int tunable_value_type;
} tunable_valid_range;

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

	uint32_t max_m_clock;
	uint32_t max_c_clock;
	uint32_t max_voltage;
} amd_pstate_info;

#endif
