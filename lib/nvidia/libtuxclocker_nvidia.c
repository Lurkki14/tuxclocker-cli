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
		default:
			return 1;
	}

}

int tc_nvidia_get_tunable_range(void *nvml_handle, void *nvctrl_handle, tunable_valid_range *range, int tunable_enum, int gpu_index) {
	switch (tunable_enum) {
		case TUNABLE_POWER_LIMIT: ; {
			uint32_t min, max;
			nvmlReturn_t retval = nvmlDeviceGetPowerManagementLimitConstraints(*(nvmlDevice_t*) nvml_handle, &min, &max);
			
			range->tunable_value_type = TUNABLE_ABSOLUTE;
			range->min = min / 1000;
			range->max = max / 1000;
			return retval;
		}
		case TUNABLE_CORE_CLOCK: ;
			tunable_enum = NV_CTRL_GPU_NVCLOCK_OFFSET;
			goto range_from_nvctrl;
		default:
			return 1;
	}
// For ranges gotten through xnvctrl only the tunable enum is changed
range_from_nvctrl: ;
	// Contains the ranges for attribute
	NVCTRLAttributeValidValuesRec range_values;
	Bool retval = XNVCTRLQueryValidTargetAttributeValues((Display*) nvctrl_handle, NV_CTRL_TARGET_TYPE_GPU, gpu_index, 0, tunable_enum, &range_values);

	if (!retval) {
		// Failure
		return 1;
	}
	// Check if attribute is writable
	if ((range_values.permissions & ATTRIBUTE_TYPE_WRITE) == ATTRIBUTE_TYPE_WRITE) {
		// Success
		range->tunable_value_type = TUNABLE_OFFSET;
		range->min = range_values.u.range.min;
		range->max = range_values.u.range.max;
		return 0;
	}
	return 1;
}
