#include <stdint.h>

namespace BitOps {
    uint8_t reverse_bits(uint8_t x);
    uint16_t reverse_bits(uint16_t x);
    unsigned reverse_bits(unsigned x, unsigned n);
    uint32_t unaligned_bitswap_le32(const uint32_t *ptr32);
}
