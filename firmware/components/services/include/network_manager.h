#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    NETWORK_TYPE_NONE,
    NETWORK_TYPE_WIFI,
    NETWORK_TYPE_LTE
} network_type_t;

esp_err_t network_manager_init(void);
network_type_t network_manager_get_active_connection(void);
bool network_manager_is_connected(void);

#endif // NETWORK_MANAGER_H
