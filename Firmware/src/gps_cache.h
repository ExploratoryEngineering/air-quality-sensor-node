/*
**   Copyright 2020 Telenor Digital AS
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#ifndef _GPS_CACHE_H_
#define _GPS_CACHE_H_
// Cache functions for GPS fix. The fix is updated at a set rate from the
// receiver and accessing the latest fix is simplified by caching the latest
// result.
#include <stdint.h>
#include <stdbool.h>

#include "gps_std.h"


// Fix information; aggregates all of the relevant fix into a single struct
typedef struct {
	float timestamp;
	float latitude;
	float longitude;
	float altitude;
	float hdop;
	float vdop;
	float pdop;
	uint8_t fix_quality;
	uint8_t gps_svs;
	uint8_t glo_svs;
	uint8_t waas_svs;
	bool moving;
	float track_angle;
} gps_fix_t;

typedef struct {
	uint8_t max_time;
	uint8_t min_time;
	uint8_t last_time;
	uint16_t timeouts;
	uint16_t samples;
	uint32_t total_time;
} gps_fix_stats_t;

// Check if fix is valid
bool gps_fix_is_valid(const gps_fix_t* fix);


// Get the latest fix. Returns false if no fix is available.
bool gps_get_fix(gps_fix_t* fix);

// Update fix with GGA data (internal?)
void gps_update_gga(gps_gga_t* gga);

// Update fix with GSA data (internal?)
void gps_update_gsa(gps_gsa_t* gsa);

// Update fix with RMC data (internal?)
void gps_update_rmc(gps_rmc_t* rmc);

// Add sample point
void gps_add_fix_stat(const int seconds_to_fix);

void gps_add_timeout_stat();

// Get fix statistics
void gps_get_fix_stat(gps_fix_stats_t* statcp);

// Reset fix cache
void gps_reset_cache(void);

typedef struct imu_data_s {
	float accel_x;
	float accel_y;
	float accel_z;
	float mag_x;
	float mag_y;
	float mag_z;
	float temperature;
} imu_data_t;

void imu_update_accel(float x, float y, float z);

void imu_update_temp(float temperature);

void imu_update_mag(float x, float y, float z);

void imu_get_data(imu_data_t* data);

#endif
