#ifndef FELIXTOOLS_H
#define FELIXTOOLS_H

#include <cstdint>
#include <bitset>

namespace FelixTools {

  using FelixID_t = uint64_t;

  FelixID_t get_fid(uint8_t detectorID, uint16_t connectorID, bool is_virtual, uint16_t linkID, uint8_t elink, bool to_felix, uint8_t protocol, uint8_t streamID);

}

#endif
