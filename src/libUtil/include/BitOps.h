#include <stdint.h>

namespace BitOps {
    inline uint8_t reverse_bits8(uint8_t x);
    uint32_t unaligned_bitswap_le32(const uint32_t *ptr32);
}
