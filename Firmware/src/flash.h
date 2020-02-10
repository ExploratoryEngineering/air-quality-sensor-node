#pragma once

int write_firmware_block(uint8_t *data, uint16_t data_len, bool last_block, size_t total_size);