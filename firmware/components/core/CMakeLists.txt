idf_component_register(SRCS "event_bus.c" "state_machine.c" "failsafe_manager.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_event hal nvs_flash app_update)
