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
#include "nmealib.h"
#include <stdio.h>

/**
 * Internal struct for housekeeping
 */
typedef struct {
    uint8_t* buffer;     /* the source buffer */
    uint8_t start;       /* start of message */
    uint8_t end;         /* end of message */
    uint8_t checksum;    /* checksum of message */
} nmea_internal_t;

static char to_hexchar(uint8_t val) {
    val &= 0x0F;
    if (val < 10) {
        return ('0' + val);
    }
    return ('A' + (val - 10));
}

/**
 * Check if this is a valid NMEA string. The string doesn't have to be trimmed.
 */
static bool nmea_valid(uint8_t* const buffer) {
    uint8_t checksum = 0;
    uint8_t end = 0;
    uint8_t checksum_pos = 0;
    bool has_checksum = false;
    bool in_sentence = false;
    bool has_preamble = false;
    for (int i = 0; end == 0 && i < MAX_NMEA_BUFFER; i++) {
        switch (buffer[i]) {
            case NMEA_PREAMBLE:
                checksum = 0;
                in_sentence = true;
                has_preamble = true;
                break;
            case NMEA_CHECKSUM:
                has_checksum = true;
                checksum_pos = i;
            case CR:
            case LF:
            case 0:
                end = i;
                break;
            default:
                if (in_sentence) {
                    checksum ^= buffer[i];
                    // Must be a printable character
                    if (buffer[i] < 32 || buffer[i] > 126) {
                        // Won't allow non-printable chars
                        return false;
                    }
                }
                break;
        }
    }

    if (!has_preamble) {
        return false;
    }
    // Verify the checksum (if it exists)
    if (has_checksum) {
        if (buffer[checksum_pos + 1] != to_hexchar((0xF0 & checksum) >> 4)) {
            return false;
        }
        if (buffer[checksum_pos + 2] != to_hexchar(0x0F & checksum)) {
            return false;
        }
    }
    return true;
}

bool nmea_parse(uint8_t* buffer, nmea_sentence_t* output) {
    if (!nmea_valid(buffer)) {
        return false;
    }
    bool in_field = false;
    uint8_t start_of_field = 0;
    bool complete = false;
    bool in_sentence = false;
    output->field_count = 0;
    output->talker[0] = 0;
    output->type = NULL;
    int i = 0;
    for (i = 0 ; !complete && i < MAX_NMEA_BUFFER; i++) {
        switch (buffer[i]) {
            case FIELD_SEPARATOR:
                if (in_field) {
                    // 0-terminate
                    buffer[i] = 0;
                    // ..and set pointer for field while incrementing
                    // the number of fields.
                    output->fields[output->field_count++]
                        = (buffer + start_of_field);
                }
                // start the new field
                start_of_field = (i + 1);
                break;
            case NMEA_PREAMBLE:
                // Start sentence processing
                in_sentence = true;
                in_field = true;
                start_of_field = i + 1;
                break;
            case NMEA_CHECKSUM:
            case CR:
            case LF:
            case 0:
                if (in_sentence) {
                    // end of string - stop processing.
                    complete = true;
                }
                break;

        }
    }
    if (in_sentence && start_of_field < i) {
        // Add the last field
        buffer[i - 1] = 0;
        output->fields[output->field_count++] = (buffer +  start_of_field);
    }

    if (output->field_count > 0) {
        if (output->fields[0][0] == 'P') {
            // This is a proprietary sentence
            output->talker[0] = 'P';
            output->talker[1] = 0;
            output->type = (output->fields[0] + 1);            
        }
        else {
            // Ordinary talker + type structure
            output->talker[0] = output->fields[0][0];
            output->talker[1] = output->fields[0][1];
            output->talker[2] = 0;
            output->type = (output->fields[0] + 2);

        }
    }
    return true;
}
