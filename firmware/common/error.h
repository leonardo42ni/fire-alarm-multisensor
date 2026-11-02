#ifndef FIRE_COMMON_ERROR_H
#define FIRE_COMMON_ERROR_H

typedef enum {
    APP_OK = 0,
    APP_ERR_INVALID_ARG = -1,
    APP_ERR_INVALID_STATE = -2,
    APP_ERR_NOT_FOUND = -3,
    APP_ERR_NO_MEM = -4,
    APP_ERR_TIMEOUT = -5,
    APP_ERR_IO = -6,
    APP_ERR_PARSE = -7,
    APP_ERR_RANGE = -8
} app_err_t;

#endif
