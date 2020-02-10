#pragma once
#include <zephyr.h>

// UART thread has priority
#define MAX_THREAD_PRIORITY (-CONFIG_NUM_COOP_PRIORITIES + 2)
// ...then the modem comm thread
#define URC_THREAD_PRIORITY (-CONFIG_NUM_COOP_PRIORITIES + 1)
#define GPS_THREAD_PRIORITY (-CONFIG_NUM_COOP_PRIORITIES + 3)
