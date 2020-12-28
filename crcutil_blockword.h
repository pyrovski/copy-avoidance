#pragma once

#include <stdint.h>

uint32_t crc32(uint32_t prev, void *data, size_t length);
