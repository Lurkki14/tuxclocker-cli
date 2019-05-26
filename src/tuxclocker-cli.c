#include <stdio.h>
#include <dlfcn.h>
//#include "lib/libamd.h"

// Library handles for different GPU vendors
void *libtc_amd = NULL;
void *libtc_nvidia;

int main() {
	libtc_amd = dlopen("libtuxclocker_amd.so", RTLD_NOW);
	if (libtc_amd != NULL) {
		printf("non-null\n");
		int (*amd_get_gpu_count)() = dlsym(libtc_amd, "tc_amd_get_gpu_count");
		printf("Found %d AMD GPUs\n", amd_get_gpu_count());
	}
	return 0;
}
