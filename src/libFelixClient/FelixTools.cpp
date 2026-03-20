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

std::string FelixTools::print_fid(uint64_t fid)
{
  std::stringstream ss;

  ss << "ver: " << (fid >> FELIXID_VER_SHIFT) << ", ";
  ss << "did: " << ((fid >> FELIXID_DID_SHIFT) & ((1<<FELIXID_DID_NBITS) - 1)) << ", ";
  ss << "cid: " << ((fid >> FELIXID_CID_SHIFT) & ((1<<FELIXID_CID_NBITS) - 1)) << ", ";
  ss << "virt: " << ((fid >> FELIXID_ISVIRT_SHIFT) & 1) << ", ";
  ss << "link: " << ((fid >> FELIXID_LINKID_SHIFT) & ((1<<FELIXID_LINKID_NBITS) - 1)) << ", ";
  ss << "elink: " << ((fid >> FELIXID_ELINK_SHIFT) & ((1<<FELIXID_ELINK_NBITS) - 1)) << ", ";
  ss << "dir: " << ((fid >> FELIXID_TOFLX_SHIFT) & 1) << ", ";
  ss << "proto: " << ((fid >> FELIXID_PROTO_SHIFT) & ((1<<FELIXID_PROTO_NBITS) - 1)) << ", ";
  ss << "stream: " << ((fid >> FELIXID_STREAM_SHIFT) & ((1<<FELIXID_STREAM_NBITS) - 1));

  return ss.str();
}

std::tuple<uint16_t,uint8_t,uint8_t> FelixTools::linkInfo_from_chn(uint32_t chn, bool toflx, FelixTools::FELIX_FW_MODE fwmode) {
  uint16_t linkId = FelixTools::link_from_chn(chn);
  uint8_t elink = FelixTools::elink_from_chn(chn);

  // A special case for Strip LCB encoder
  bool isLCB = toflx and fwmode == FelixTools::FELIX_FW_MODE::ITK_Strip;
  uint8_t egroup = isLCB ? FelixTools::egroup_from_elink_lcb(elink) : FelixTools::egroup_from_elink(elink);
  uint8_t epath = isLCB ? FelixTools::epath_from_elink_lcb(elink) : FelixTools::epath_from_elink(elink);

  // return linkId, egroup, epath
  return std::make_tuple(linkId, egroup, epath);
}

std::tuple<uint16_t,uint8_t,uint8_t,bool> FelixTools::linkInfo_from_fid(FelixTools::FelixID_t fid, FelixTools::FELIX_FW_MODE fwmode) {
  uint16_t linkId = FelixTools::link_from_fid(fid);
  uint8_t elink = FelixTools::elink_from_fid(fid);
  bool toflx = FelixTools::toflx_from_fid(fid);

  // Special case for Strip LCB encoder
  bool isLCB = toflx and fwmode == FelixTools::FELIX_FW_MODE::ITK_Strip;
  uint8_t egroup = isLCB ? FelixTools::egroup_from_elink_lcb(elink) : FelixTools::egroup_from_elink(elink);
  uint8_t epath = isLCB ? FelixTools::epath_from_elink_lcb(elink) : FelixTools::epath_from_elink(elink);

  // return linkId, egroup, epath, toflx
  return std::make_tuple(linkId, egroup, epath, toflx);
}

std::tuple<uint32_t,uint32_t,uint32_t> FelixTools::lcbChns_from_chn(uint32_t chn) {
  uint32_t linkId = link_from_chn(chn);
  uint32_t elink = elink_from_chn(chn);
  uint32_t segment = egroup_from_elink_lcb(elink);
  uint32_t lcb_command = (linkId << BLOCK_LNK_SHIFT) + segment * 5 + 1;
  uint32_t lcb_config = lcb_command - 1;
  uint32_t lcb_trickle = lcb_command + 1;
  return std::make_tuple(lcb_config, lcb_command, lcb_trickle);
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

std::string FelixTools::getELinkRegName(unsigned linkId, unsigned egroup, bool toflx, const std::string& suffix) {
  std::stringstream regName;
  if (toflx) {
    regName << "ENCODING_LINK";
  } else {
    regName << "DECODING_LINK";
  }
  regName << std::setfill('0') << std::setw(2) << linkId << "_EGROUP" << egroup << "_CTRL_EPATH_" << suffix;
  return regName.str();
}

std::string FelixTools::getELinkRegName(FelixID_t fid, FELIX_FW_MODE fwmode, const std::string& suffix) {
  auto [linkId, egroup, epath, toflx] = linkInfo_from_fid(fid, fwmode);
  return getELinkRegName(linkId, egroup, toflx, suffix);
}

std::string FelixTools::getELinkEnableRegName(unsigned linkId, unsigned egroup, bool toflx) {
  return getELinkRegName(linkId, egroup, toflx, "ENA");
}

std::string FelixTools::getELinkEnableRegName(FelixID_t fid, FELIX_FW_MODE fwmode) {
  return getELinkRegName(fid, fwmode, "ENA");
}

std::string FelixTools::getELinkWidthRegName(unsigned linkId, unsigned egroup, bool toflx) {
  return getELinkRegName(linkId, egroup, toflx, "WIDTH");
}

std::string FelixTools::getELinkWidthRegName(FelixID_t fid, FELIX_FW_MODE fwmode) {
  return getELinkRegName(fid, fwmode, "WIDTH");
}

std::string FelixTools::getLinkPathEncodingRegName(unsigned linkId, unsigned egroup, bool toflx) {
  std::stringstream regName;
  if (toflx) {
    regName << "ENCODING_LINK";
  } else {
    regName << "DECODING_LINK";
  }
  regName << std::setfill('0') << std::setw(2) << linkId << "_EGROUP" << egroup << "_CTRL_PATH_" << "ENCODING";
  return regName.str();
}

std::vector<std::string> FelixTools::getAllICEnableRegNames(bool toflx) {
  std::vector<std::string> allRegNames;
  for (unsigned l = 0; l < FelixTools::FLX_LINKS; ++l) {
    allRegNames.push_back(getICEnableRegName(l, toflx));
  }
  return allRegNames;
}

std::vector<std::string> FelixTools::getAllECEnableRegNames(bool toflx) {
  std::vector<std::string> allRegNames;
  for (unsigned l = 0; l < FelixTools::FLX_LINKS; ++l) {
    allRegNames.push_back(getECEnableRegName(l, toflx));
  }
  return allRegNames;
}

std::vector<std::string> FelixTools::getAllELinkEnableRegNames(bool toflx) {
  std::vector<std::string> allRegNames;

  unsigned NUM_EGROUPS = toflx ? FelixTools::FLX_TOFLX_EGROUPS : FelixTools::FLX_TOHOST_EGROUPS;

  for (unsigned l = 0; l < FelixTools::FLX_LINKS; ++l) {
    for (unsigned g = 0; g < NUM_EGROUPS; ++g) {
      allRegNames.push_back(getELinkEnableRegName(l,g, toflx));
    }
  }

  return allRegNames;
}