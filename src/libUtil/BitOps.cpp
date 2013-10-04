#include <stdint.h>

namespace BitOps {
    inline uint8_t reverse_bits8(uint8_t x)
    {
        x = ((x >> 1) & 0x55) | ((x & 0x55) << 1);
        x = ((x >> 2) & 0x33) | ((x & 0x33) << 2);
        x = ((x >> 4) & 0x0f) | ((x & 0x0f) << 4);

        return x;
    }

    uint32_t unaligned_bitswap_le32(const uint32_t *ptr32)
    {
        uint32_t tmp32;
        uint8_t *tmp8 = (uint8_t *) &tmp32;
        uint8_t *ptr8;

        ptr8 = (uint8_t *) ptr32;

        *(tmp8 + 0) = reverse_bits8(*(ptr8 + 0));
        *(tmp8 + 1) = reverse_bits8(*(ptr8 + 1));
        *(tmp8 + 2) = reverse_bits8(*(ptr8 + 2));
        *(tmp8 + 3) = reverse_bits8(*(ptr8 + 3));

        return tmp32;
    }
}
