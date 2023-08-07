#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>

#include "hardware.h"

#define HAL_LIBRARY_PATH1 "./libcamera.so"

static int load(const struct hw_module_t **pHmi)
{
    int status = 0;
    void *lib_handle = NULL;
    struct hw_module_t *hmi = NULL;

    lib_handle = dlopen(HAL_LIBRARY_PATH1, RTLD_NOW);
    if (lib_handle == NULL) {
        perror("dlopen err");
        status = -EINVAL;
        return status;
    }

    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    hmi = (struct hw_module_t *)dlsym(lib_handle, sym);
    if (hmi == NULL) {
        perror("dlsym err");
        status = -EINVAL;
        dlclose(lib_handle);
        return status;
    }
    printf("loaded HAL hmi=%p handle=%p", *pHmi, lib_handle);
    *pHmi = hmi;
    
    return status;
}

int hw_get_camera_module(const struct hw_module_t **module)
{
    return load(module);
}
