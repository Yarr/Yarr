#include "FelixTools.h"
#include <sstream>
#include <iomanip>

uint64_t FelixTools::get_fid(
  uint8_t detectorID,
  uint16_t connectorID,
  bool is_virtual,
  uint16_t linkID,
  uint8_t elink,
  bool to_felix,
  uint8_t protocol,
  uint8_t streamID
  )
{
  using namespace FelixTools;

  std::bitset<64> fid;
  std::bitset<64> id;

  // Version [63:60] is always 0x1
  id = 1;
  fid |= (id << FELIXID_VER_SHIFT);

  // Detector ID
  id = detectorID & ((1<<FELIXID_DID_NBITS) - 1);
  fid |= (id << FELIXID_DID_SHIFT);

  // Connector ID
  id = connectorID & ((1<<FELIXID_CID_NBITS) - 1);
  fid |= (id << FELIXID_CID_SHIFT);

  // Is virtual link
  // assert(FELIXID_ISVIRT_NBITS == 1);
  fid[FELIXID_ISVIRT_SHIFT] = is_virtual;

  // Link (GBT) ID
  id = linkID & ((1<<FELIXID_LINKID_NBITS) - 1);
  fid |= (id << FELIXID_LINKID_SHIFT);

  // E-link
  id = elink & ((1<<FELIXID_ELINK_NBITS) - 1);
  fid |= (id << FELIXID_ELINK_SHIFT);

  // Link direction
  // assert(FELIXID_TOFLX_NBITS==1);
  fid[FELIXID_TOFLX_SHIFT] = to_felix;

  // Protocol
  id = protocol & ((1<<FELIXID_PROTO_NBITS) - 1);
  fid |= (id << FELIXID_PROTO_SHIFT);

  // Stream ID
  id = streamID & ((1<<FELIXID_STREAM_NBITS) - 1);
  fid |= (id << FELIXID_STREAM_SHIFT);

  return fid.to_ullong();
}

std::string FelixTools::getICEnableRegName(uint16_t linkId, bool toflx) {
  std::stringstream regName;
  regName << "MINI_EGROUP_";
  if (toflx) {
    regName << "FROMHOST_";
  } else {
    regName << "TOHOST_";
  }
  regName << std::setfill('0') << std::setw(2) << linkId << "_IC_ENABLE";
  return regName.str();
}

std::string FelixTools::getICEnableRegName(FelixID_t fid) {
  return FelixTools::getICEnableRegName(
    FelixTools::link_from_fid(fid),
    FelixTools::toflx_from_fid(fid)
  );
}

std::string FelixTools::getECEnableRegName(uint16_t linkId, bool toflx) {
  std::stringstream regName;
  regName << "MINI_EGROUP_";
  if (toflx) {
    regName << "FROMHOST_";
  } else {
    regName << "TOHOST_";
  }
  regName << std::setfill('0') << std::setw(2) << linkId << "_EC_ENABLE";
  return regName.str();
}

std::string FelixTools::getECEnableRegName(FelixID_t fid) {
  return FelixTools::getECEnableRegName(
    FelixTools::link_from_fid(fid),
    FelixTools::toflx_from_fid(fid)
  );
}

std::string FelixTools::getELinkEnableRegName(uint16_t linkId, uint8_t egroup, bool toflx) {
  std::stringstream regName;
  if (toflx) {
    regName << "ENCODING_LINK";
  } else {
    regName << "DECODING_LINK";
  }
  regName << std::setfill('0') << std::setw(2) << linkId << "_EGROUP" << egroup << "_CTRL_EPATH_ENA";
  return regName.str();
}

std::string FelixTools::getELinkEnableRegName(FelixID_t fid) {
  bool toflx = FelixTools::toflx_from_fid(fid);
  uint16_t linkId = FelixTools::link_from_fid(fid);
  uint8_t elink = FelixTools::elink_from_fid(fid);
  uint8_t egroup = FelixTools::egroup_from_elink(elink);

  return FelixTools::getELinkEnableRegName(linkId, egroup, toflx);
}