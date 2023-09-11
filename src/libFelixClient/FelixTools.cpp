#include "FelixTools.h"

// cf. https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/docs/LinkMappingSpecification.pdf, Figure 3
uint64_t FelixTools::get_fid( // Version [63:60] always 0x1
  uint8_t detectorID,   // [59:52]
  uint16_t connectorID, // [51:36]
  bool is_virtual,      // [35]
  uint16_t linkID,      // [34:22], 13 bits
  uint8_t elink,        // [21:16]
  bool to_felix,        // [15]
  uint8_t protocol,     // [14:8]
  uint8_t streamID      // [7:0]
  )
{
  std::bitset<64> fid;
  std::bitset<64> id;

  // Version [63:60] is always 0x1
  id = 1;
  fid |= (id << 60);

  // Detector ID [59:52], 8 bits
  id = detectorID;
  fid |= (id << 52);

  // Connector ID [51:36], 16 bits
  id = connectorID;
  fid |= (id << 36);

  // Is virtual link [35], 1 bit
  fid[35] = is_virtual;

  // Link (GBT) ID [34:22], 13 bits
  id = linkID & 0x1fff;
  fid |= (id << 22);

  // E-link [21:16], 6 bits
  id = elink & 0x3f;
  fid |= (id << 16);

  // Link direction [15] (0 = to host; 1 = to FELIX), 1 bit
  fid[15] = to_felix;

  // Protocol [14:8], 7 bits
  // cf. https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/docs/LinkMappingSpecification.pdf, Table 2
  id = protocol & 0x7f;
  fid |= (id << 8);

  // Stream ID [7:0], 8 bits
  id = streamID;
  fid |= id;

  return fid.to_ullong();
}

unsigned FelixTools::link_from_fid(uint64_t fid) {
  // bit [34:22]
  return (fid >> 22) & 0x1fff;
}

unsigned FelixTools::elink_from_fid(uint64_t fid) {
  // bit [21:16]
  return (fid >> 16) & 0x3f;
}