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

// Standard GPS sentence decoding. GGA and GSA is the most relevant alternatives
// to decode. GSV, RMC and GSV plus ZDA might be relevant later
#ifndef GPS_STD_H
#define GPS_STD_H

#include <stdint.h>

#include "nmealib.h"

#define M_PI 3.14159265359

#define DEG_TO_RAD(x) ((x)*(M_PI/180.0f))
#define RAD_TO_DEG(x) ((x) * (180.0f/M_PI))

#define FIX_Q_INVALID 0
#define FIX_Q_GPS 1
#define FIX_Q_DGPS 2
#define FIX_Q_PPS 3
#define FIX_Q_RTK 4
#define FIX_Q_FLOAT_RTK 5
#define FIX_Q_ESTIMATED 6
#define FIX_Q_MANUAL 7
#define FIX_Q_SIM 8

#define FIX_NONE 1
#define FIX_2D 2
#define FIX_3D 3

#define MAX_GSA_PRNS 12


// GGA: Essiential fix data
typedef struct {
	float timestamp;      // time (HHMMSS.fff) 
	float latitude;       // latitude (positive: north, negative: south) 
	float longitude;      // longitude (positive: east, negative: west) 
	uint8_t fix_quality;  // fix quality, see constants 
	uint8_t num_svs;      // number of satellites 
	float hdop;           // height dilution of precision 
	float alt;            // altitude (over sea level), meters 
	float geoid_height;   // height over geoid (usually WGS84), meters 
} gps_gga_t;

// GSA: GPS DOP and active satellites
typedef struct {
	uint8_t auto_selection;      // auto selection 3D fix ('A') or manual ('M') 
	uint8_t fix_type;            // Fix type, see constants 
	uint8_t prns[MAX_GSA_PRNS];  // PRNs for satellites used (max 12) 
	float pdop;                  // precision dilution of position 
	float vdop;                  // vertical dilution of precision 
} gps_gsa_t;

typedef struct {
	float speed;	// Speed over the ground in knots 
	bool active;	// Void / Active 
	bool moving;	// defined as "speed > 20 km/h" 
	float track_angle;
} gps_rmc_t;



// Decode GGA sentence.
void nmea_decode_gga(const nmea_sentence_t* param, gps_gga_t* output);

// Decode GSA sentence.
void nmea_decode_gsa(const nmea_sentence_t* param, gps_gsa_t* output);

// Decode RMC sentence.
void nmea_decode_rmc(const nmea_sentence_t* param, gps_rmc_t* output);
#endif
