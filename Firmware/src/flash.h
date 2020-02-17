#pragma once

int write_firmware_block(const uint8_t *data, uint16_t data_len, bool first_block, bool last_block, size_t total_size);