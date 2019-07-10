#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrlLib.h>

#include "../include/nvml.h"
#include "libtuxclocker_nvidia.h"

int tc_nvidia_get_gpu_count(uint8_t *gpu_count) {
        nvmlReturn_t retval = nvmlInit();
        if (retval != NVML_SUCCESS) {
                // Couldn't init NVML
                return 1;
        }
        uint32_t dev_count = 0;
        retval = nvmlDeviceGetCount(&dev_count);
        if (retval == NVML_SUCCESS) {
                *gpu_count = dev_count;
        }
	return retval;
}

int tc_nvidia_get_nvml_handle_by_index(void **nvml_handle, int index) {
	*nvml_handle = malloc(sizeof(nvmlDevice_t));
	nvmlReturn_t retval = nvmlDeviceGetHandleByIndex(index, (nvmlDevice_t*) *nvml_handle);

	if (retval != NVML_SUCCESS) {
		// Free memory on failure
		free(*nvml_handle);
	}
	return retval;
}

int tc_nvidia_get_nvctrl_handle(void **nvctrl_handle) {
	*nvctrl_handle = XOpenDisplay(NULL);
	if (*nvctrl_handle == NULL) {
		// Couldn't get X display
		return 1;
	}
	// Check if the nvidia extension is present
	int event_basep, error_basep;
	Bool retval = XNVCTRLQueryExtension((Display*) *nvctrl_handle, &event_basep, &error_basep);
	if (!retval) {
		// Extension wasn't found
		return 1;
	}
	// Success
	return 0;
}

int tc_nvidia_get_nvml_gpu_name(void *nvml_handle, char (*name)[], size_t str_len) {
	nvmlReturn_t retval = nvmlDeviceGetName(*(nvmlDevice_t*) nvml_handle, *name, str_len);
	return retval;
}

int tc_nvidia_get_sensor_value(void *nvml_handle, void *nvctrl_handle, sensor_info *info, int sensor_enum, int gpu_index) {
	// Check if the reading is from NVML or NVCtrl and what function is called
	switch (sensor_enum) {
		case SENSOR_POWER_DRAW: ; {
			if (nvml_handle == NULL) {
				return 1;
			}

			info->sensor_data_type = SENSOR_TYPE_DOUBLE;
			uint32_t reading = 0;
			nvmlReturn_t retval = nvmlDeviceGetPowerUsage(*(nvmlDevice_t*) nvml_handle, &reading);
			// Divide by 1000 to get usage in watts
			info->readings.d_reading = (double) reading / 1000;
			return retval;
		}
		case SENSOR_TEMP: ;
			info->sensor_data_type = SENSOR_TYPE_UINT;
			return nvmlDeviceGetTemperature(*(nvmlDevice_t*) nvml_handle, NVML_TEMPERATURE_GPU, &(info->readings.u_reading));
		case SENSOR_FAN_PERCENTAGE: ;
			info->sensor_data_type = SENSOR_TYPE_UINT;
			return nvmlDeviceGetFanSpeed(*(nvmlDevice_t*) nvml_handle, &(info->readings.u_reading));
		case SENSOR_CORE_CLOCK: ;
			info->sensor_data_type = SENSOR_TYPE_UINT;
			return nvmlDeviceGetClock(*(nvmlDevice_t*) nvml_handle, NVML_CLOCK_GRAPHICS, NVML_CLOCK_ID_CURRENT, &(info->readings.u_reading));
		case SENSOR_MEMORY_CLOCK: ;
			info->sensor_data_type = SENSOR_TYPE_UINT;
			return nvmlDeviceGetClock(*(nvmlDevice_t*) nvml_handle, NVML_CLOCK_MEM, NVML_CLOCK_ID_CURRENT, &(info->readings.u_reading));
		case SENSOR_CORE_UTILIZATION: ; {
			info->sensor_data_type = SENSOR_TYPE_UINT;
			nvmlUtilization_t utils;
			nvmlReturn_t retval = nvmlDeviceGetUtilizationRates(*(nvmlDevice_t*) nvml_handle, &utils);
			info->readings.u_reading = utils.gpu;
			return retval;
		}
		case SENSOR_MEMORY_UTILIZATION: ; {
			info->sensor_data_type = SENSOR_TYPE_UINT;
			nvmlUtilization_t utils;
			nvmlReturn_t retval = nvmlDeviceGetUtilizationRates(*(nvmlDevice_t*) nvml_handle, &utils);
			info->readings.u_reading = utils.memory;
			return retval;
		}
		case SENSOR_CORE_VOLTAGE: ; {
			info->sensor_data_type = SENSOR_TYPE_DOUBLE;
			int reading = 0;
			Bool retval = XNVCTRLQueryTargetAttribute((Display*) nvctrl_handle, NV_CTRL_TARGET_TYPE_GPU, gpu_index, 0, NV_CTRL_GPU_CURRENT_CORE_VOLTAGE, &reading);
			// Divide by 1000 to get millivolts
			info->readings.d_reading = (double) reading / 1000;
			
			if (retval) {
				// Success
				return 0;
			}
			return 1;
		}
		case SENSOR_MEMORY_MB_USAGE: {
			info->sensor_data_type = SENSOR_TYPE_UINT;
			int reading = 0;
			Bool retval = XNVCTRLQueryTargetAttribute((Display*) nvctrl_handle, NV_CTRL_TARGET_TYPE_GPU, gpu_index, 0, NV_CTRL_USED_DEDICATED_GPU_MEMORY, &reading);
			info->readings.u_reading = reading;
			if (!retval) {
				return 1;
			}
			return 0;
		}
		default:
			return 1;
	}

}

int tc_nvidia_get_tunable_range(void *nvml_handle, void *nvctrl_handle, tunable_valid_range *range, int tunable_enum, int gpu_index, int pstate_index) {
	switch (tunable_enum) {
		case TUNABLE_POWER_LIMIT: ; {
			uint32_t min, max;
			nvmlReturn_t retval = nvmlDeviceGetPowerManagementLimitConstraints(*(nvmlDevice_t*) nvml_handle, &min, &max);
			
			range->tunable_value_type = TUNABLE_ABSOLUTE;
			range->min = min / 1000;
			range->max = max / 1000;
			return retval;
		}
		case TUNABLE_CORE_VOLTAGE: ;
			tunable_enum = NV_CTRL_GPU_OVER_VOLTAGE_OFFSET;
			goto range_from_nvctrl;
		case TUNABLE_CORE_CLOCK: ;
			tunable_enum = NV_CTRL_GPU_NVCLOCK_OFFSET;
			goto range_from_nvctrl;
		case TUNABLE_MEMORY_CLOCK: ;
			tunable_enum = NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET;
			goto range_from_nvctrl;
		case TUNABLE_FAN_SPEED_PERCENTAGE:
			tunable_enum = NV_CTRL_GPU_COOLER_MANUAL_CONTROL;
			goto range_from_nvctrl;
		default:
			return 1;
	}
// For ranges gotten through xnvctrl only the tunable enum is changed
range_from_nvctrl: ;
	// Contains the ranges for attribute
	NVCTRLAttributeValidValuesRec range_values;
	Bool retval = XNVCTRLQueryValidTargetAttributeValues((Display*) nvctrl_handle, NV_CTRL_TARGET_TYPE_GPU, gpu_index, pstate_index, tunable_enum, &range_values);

	if (!retval) {
		// Failure
		return 1;
	}
	// Check if attribute is writable
	if ((range_values.permissions & ATTRIBUTE_TYPE_WRITE) == ATTRIBUTE_TYPE_WRITE) {
		// Success
		range->tunable_value_type = TUNABLE_OFFSET;
		// Convert some units to match the units in libtuxclocker.h
		switch (tunable_enum) {
			case NV_CTRL_GPU_OVER_VOLTAGE_OFFSET: ;
				range->min = range_values.u.range.min / 1000;
				range->max = range_values.u.range.max / 1000;
				return 0;
			case NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET: ;
				range->min = range_values.u.range.min / 2;
				range->max = range_values.u.range.max / 2;
				return 0;
			case NV_CTRL_GPU_COOLER_MANUAL_CONTROL:
				range->tunable_value_type = TUNABLE_ABSOLUTE;
				range->min = 0;
				range->max = 100;
				return 0;
			default:
				range->min = range_values.u.range.min;
				range->max = range_values.u.range.max;
				return 0;
		}
	}
	return 1;
}

int tc_nvidia_get_pstate_count(void *nvml_handle, int *pstate_count) {
	// novideo didn't bother to make a separate function for this
	uint32_t *clocks;
	uint32_t amount = 0;
	nvmlReturn_t retval = nvmlDeviceGetSupportedMemoryClocks(*(nvmlDevice_t*) nvml_handle, &amount, clocks);
	*pstate_count = amount;
	return retval;
}

int tc_nvidia_assign_value(void *nvml_handle, void *nvctrl_handle, int tunable_enum, int target_value, int gpu_index, int pstate_index) {
	// Target enum for xnvctrl
	int target_enum = 0;
	switch (tunable_enum) {
		case TUNABLE_POWER_LIMIT:
			return nvmlDeviceSetPowerManagementLimit(*(nvmlDevice_t*) nvml_handle, (uint32_t) target_value * 1000);
		case TUNABLE_CORE_CLOCK: ;
			tunable_enum = NV_CTRL_GPU_NVCLOCK_OFFSET;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			break;
		case TUNABLE_FAN_MODE: ;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			switch (target_value) {
				case FAN_MODE_MANUAL: ;
					tunable_enum = NV_CTRL_GPU_COOLER_MANUAL_CONTROL;
					target_value = NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE;
					break;
				case FAN_MODE_AUTO: ;
					tunable_enum = NV_CTRL_GPU_COOLER_MANUAL_CONTROL;
					target_value = NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE;
					break;
				default:
					return 1;
			}
			break;
		case TUNABLE_MEMORY_CLOCK:
			tunable_enum = NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			// Memory transfer rate is twice the clock speed
			target_value *= 2;
			break;
		case TUNABLE_FAN_SPEED_PERCENTAGE: ;
			tunable_enum = NV_CTRL_THERMAL_COOLER_LEVEL;
			target_enum = NV_CTRL_TARGET_TYPE_COOLER;
			break;
		case TUNABLE_CORE_VOLTAGE:
			tunable_enum = NV_CTRL_GPU_OVER_VOLTAGE_OFFSET;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			// Convert to microwatts	
			target_value *= 1000;
			break;
		default:
			return 1;
	}
	// Values assigned with xnvctrl
	Bool retval = XNVCTRLSetTargetAttributeAndGetStatus((Display*) nvctrl_handle, target_enum, gpu_index, pstate_index, tunable_enum, target_value);
	if (!retval) {
		return 1;
	}
	return 0;
}

char *tc_nvidia_nvml_error_string_from_retval(int nvml_retval) {
	return nvmlErrorString(nvml_retval);
}

int tc_nvidia_get_property_value(void *nvml_handle, void *nvctrl_handle, sensor_info *info, int prop_enum, int gpu_index) {
	int target_enum = 0;
	switch (prop_enum) {
		case PROPERTY_TOTAL_VRAM:
			prop_enum = NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		// Broken
		/*
		case PROPERTY_THROTTLE_TEMP:
			prop_enum = NV_CTRL_GPU_CORE_THRESHOLD;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		*/
		case PROPERTY_PCIE_GEN:
			prop_enum = NV_CTRL_GPU_PCIE_GENERATION;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		case PROPERTY_GPU_CORE_COUNT:
			prop_enum = NV_CTRL_GPU_CORES;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		case PROPERTY_MEM_BUS_WIDTH:
			prop_enum = NV_CTRL_GPU_MEMORY_BUS_WIDTH;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		case PROPERTY_PCIE_MAX_LINK_SPEED:
			prop_enum = NV_CTRL_GPU_PCIE_MAX_LINK_SPEED;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		case PROPERTY_PCIE_LINK_WIDTH:
			prop_enum = NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		case PROPERTY_PCIE_CUR_LINK_SPEED:
			prop_enum = NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED;
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			goto int_value_from_nvctrl;
		default:
			return 1;
	}
	return 1;
int_value_from_nvctrl:
	info->sensor_data_type = SENSOR_TYPE_UINT;
	int reading = 0;
	Bool retval = XNVCTRLQueryTargetAttribute((Display*) nvctrl_handle, target_enum, gpu_index, 0, prop_enum, &reading);
	
	if (!retval) {
		return 1;
	}
	// Change some values to match defined units
	switch (prop_enum) {
		case NV_CTRL_GPU_PCIE_MAX_LINK_SPEED:
			info->readings.u_reading = reading / 1000;
			break;
		case NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED:
			info->readings.u_reading = reading / 1000;
			break;
		default:
			info->readings.u_reading = reading;
			break;
	}	
	return 0;
}

int tc_nvidia_get_tunable_value(void *nvml_handle, void *nvctrl_handle, int *tunable_value, int tunable_enum, int gpu_index, int pstate_index) {
	int target_enum = 0;
	switch (tunable_enum) {
		case TUNABLE_POWER_LIMIT: ;
			uint32_t reading = 0;
			nvmlReturn_t retval = nvmlDeviceGetPowerManagementLimit(*(nvmlDevice_t*) nvml_handle, &reading);
			*tunable_value = reading / 1000;
			return retval;
		case TUNABLE_CORE_CLOCK:
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			tunable_enum = NV_CTRL_GPU_NVCLOCK_OFFSET;
			goto value_from_nvctrl;
		case TUNABLE_MEMORY_CLOCK:
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			tunable_enum = NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET;
			goto value_from_nvctrl;
		case TUNABLE_CORE_VOLTAGE:
			target_enum = NV_CTRL_TARGET_TYPE_GPU;
			tunable_enum = NV_CTRL_GPU_OVER_VOLTAGE_OFFSET;
			goto value_from_nvctrl;
		default:
			return 1;
	}
value_from_nvctrl: ;
	Bool retval = XNVCTRLQueryTargetAttribute((Display*) nvctrl_handle, target_enum, gpu_index, pstate_index, tunable_enum, tunable_value);
	if (!retval) {
		return 1;
	}
	// Convert some values
	switch (tunable_enum) {
		case NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET:
			*tunable_value /= 2;
			break;
		case NV_CTRL_GPU_OVER_VOLTAGE_OFFSET:
			*tunable_value /= 1000;
			break;
		default:
			break;
	}
	return 0;
}
