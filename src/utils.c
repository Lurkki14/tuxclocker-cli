#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

bool is_module_loaded(const char *module_name) {
	const char *mod_list_path = "/proc/modules";
	
	FILE *mod_list_file = NULL;
	if ((mod_list_file = fopen(mod_list_path, "r")) == NULL) {
		return false;
	}

	// Check the file for module_name
	size_t line_len = 0;
	char *line = NULL;
	while (getline(&line, &line_len, mod_list_file) != -1) {
		if (strstr(line, module_name) != NULL) {
			free(line);
			return true;
		}
	}
	free(line);
	return false;
}
