#ifndef ITSDAQ_CHECKSUM_H
#define ITSDAQ_CHECKSUM_H

#include <cstdint>

uint16_t calculate_checksum_range(const uint8_t *const begin, const uint8_t *const end);

#endif
