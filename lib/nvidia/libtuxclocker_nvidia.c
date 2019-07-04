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
	*nvctrl_handle = malloc(sizeof(Display*));
	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		// Couldn't get X display
		free(*nvctrl_handle);
		return 1;
	}
	// Check if the nvidia extension is present
	int *event_basep = NULL, *error_basep = NULL;
	Bool retval = XNVCTRLQueryExtension(dpy, event_basep, error_basep);
	if (!retval) {
		// Extension wasn't found
		free(*nvctrl_handle);
		return 1;
	}
	// Success
	*nvctrl_handle = dpy;
	return 0;
}

int tc_nvidia_get_nvml_gpu_name(void *nvml_handle, char (*name)[], size_t str_len) {
	nvmlReturn_t retval = nvmlDeviceGetName(*(nvmlDevice_t*) nvml_handle, *name, str_len);
	return retval;
}

int tc_nvidia_get_sensor_value(void *nvml_handle, void *nvctrl_handle, sensor_info *info, int sensor_enum) {
	// Check if the reading is from NVML or NVCtrl and what function is called
	switch (sensor_enum) {
		case SENSOR_POWER_DRAW: ; {
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
		/*case SENSOR_CORE_VOLTAGE: ; {
			info->sensor_data_type = SENSOR_TYPE_UINT;
			Bool retval = XNVCTRLQueryTargetAttribute((Display*) nvctrl_handle, NV_CTRL_TARGET_TYPE_GPU, 0, 0, NV_CTRL_GPU_CURRENT_CORE_VOLTAGE, &(info->readings.u_reading));
			return retval;
		}*/
		default:
			return 1;
	}

}
