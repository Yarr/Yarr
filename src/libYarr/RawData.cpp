#include "RawData.h"

RawData::RawData(uint32_t arg_adr, uint32_t *arg_buf, unsigned arg_words) {
    adr = arg_adr;
    buf = arg_buf;
    words = arg_words;
}

RawData::~RawData() {
    //delete[] buf;
}
