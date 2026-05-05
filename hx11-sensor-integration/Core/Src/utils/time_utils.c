/*
 * time_utils.c
 *
 */

#include "time_utils.h"

void convert_millis_to_hms(uint32_t total_milliseconds, uint32_t* hours, uint32_t* minutes, uint32_t* seconds) {
    uint32_t total_seconds = total_milliseconds / 1000;

    *seconds = total_seconds % 60;
    *minutes = (total_seconds / 60) % 60;
    *hours = total_seconds / 3600;
}

