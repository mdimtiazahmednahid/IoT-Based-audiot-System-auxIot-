#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "esp_err.h"

typedef enum {
    STATE_BOOT,
    STATE_HARDWARE_INIT,
    STATE_SELF_TEST,
    STATE_NETWORK_INIT,
    STATE_READY,
    STATE_AUDIO_ACTIVE,
    STATE_ERROR,
    STATE_LOW_BATTERY,
    STATE_SHUTDOWN
} auxiot_system_state_t;

/**
 * @brief Initialize the state machine
 */
esp_err_t state_machine_init(void);

/**
 * @brief Start the state machine loop/task
 */
esp_err_t state_machine_start(void);

/**
 * @brief Get the current system state
 */
auxiot_system_state_t state_machine_get_current_state(void);

#endif // STATE_MACHINE_H
