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

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <logging/log.h>
#include "gps_std.h"
#include "nmealib.h"

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_DECLARE(EE06);

// Convert a string into float. Empty string is interpreted as 0, trims
// leading zeroes from the string before converting.
static float nmea_atof(uint8_t* str) {
	if (str == NULL || str[0] == 0) {
		return 0.0f;
	}
	// Skip leading zeroes for string
	char* start = (char *)str;
	while (start[0] == '0') start++;
	return atof(start);
}

// Convert string into integer. Empty string is interpreted as 0, trims
// leading zeroes from string before converting.
static int nmea_atoi(uint8_t* str) {
	if (str == NULL || str[0] == 0) {
		return 0;
	}
	char* start = (char *)str;
	while (start[0] == '0') start++;
	return atoi(start);
}

// Convert degrees and minutes into radians.
static float nmea_deg_min_to_rad(float deg, float min) {
	min /= 60.0f;
	return DEG_TO_RAD(deg + min);
}

// Convert latitude string (DD.MMMM) into radians.
static float nmea_latof(uint8_t* str) {
	if (str[0] == 0) {
		return 0.0f;
	}
	float degrees = ((str[0] - '0') * 10.0f) + (str[1] - '0') * 1.0f;
	float minutes = nmea_atof(str + 2);
	return nmea_deg_min_to_rad(degrees, minutes);
}

// Convert longitude string (DDD.MMMM) into radians.
static float nmea_lotof(uint8_t* str) {
	if (str[0] == 0) {
		return 0.0f;
	}
	float degrees = (str[0] - '0') * 100.0f
			+ (str[1] - '0') * 10.0f
			+ (str[2] - '0') * 1.0f;
	float minutes = nmea_atof(str + 3);
	return nmea_deg_min_to_rad(degrees, minutes);
}

void nmea_decode_rmc(const nmea_sentence_t* param, gps_rmc_t* output) 
{
	output->active = (param->fields[3][0] == 'A') ? true : false;
	if (output->active) {
			output->speed = nmea_atof(param->fields[6])*1.852;
//		output->moving = output->speed > 20 ? true : false;
		output->moving = output->speed > 3 ? true : false;
		output->track_angle = nmea_atof(param->fields[7]);
	}
}

void nmea_decode_gga(const nmea_sentence_t* param, gps_gga_t* output) {
	if (param->field_count < 13) {
		return;
	}

	output->timestamp = nmea_atof(param->fields[1]);

	// Latitude and longitude is in DDMM.MMMM/DDDMM.MMMM.
	output->latitude = nmea_latof(param->fields[2]);
	// Convert sign if required
	if (param->fields[3][0] == 'S') {
		output->latitude = -output->latitude;
	}
	// Same story for longitude
	output->longitude = nmea_lotof(param->fields[4]);
	if (param->fields[5][0] == 'W') {
		output->longitude = -output->longitude;
	}
	output->fix_quality = nmea_atoi(param->fields[6]);
	output->num_svs = nmea_atoi(param->fields[7]);
	output->hdop = nmea_atof(param->fields[8]);
	output->alt = nmea_atof(param->fields[9]);
	// output is in meters; skip field
	output->geoid_height = nmea_atof(param->fields[11]);
}

void nmea_decode_gsa(const nmea_sentence_t* param, gps_gsa_t* output) {
	if (param->field_count < 18) {
		return;
	}
	output->auto_selection = param->fields[1][0];
	output->fix_type = nmea_atoi(param->fields[2]);
	// This is a bit dirty. The PRN might not be available but there's no
	// satellite with the PRN 00 (yet). It might be in a very very long time.
	for (int i = 3; i < (3 + MAX_GSA_PRNS); i++) {
		output->prns[i - 3] = nmea_atoi(param->fields[i]);
	}
	output->pdop = nmea_atof(param->fields[15]);
	output->vdop = nmea_atof(param->fields[16]);
}
