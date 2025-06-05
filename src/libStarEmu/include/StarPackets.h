#ifndef YARR_STAR_PACKETS
#define YARR_STAR_PACKETS

namespace StarPackets {

enum class PacketTypes {
  PR = 1, LP = 2, ABCRegRd = 4, ABCPacketTransRegRd = 7, HCCRegRd = 8,
  ABCFullTransRegRd = 11, ABCHPR = 13, HCCHPR = 14
};

/// Build physics data packet
std::vector<uint8_t> buildPhysicsPacket
  (const std::vector<std::pair<unsigned, std::vector<uint16_t>>>& clusters,
   PacketTypes typ, uint8_t l0_tag, uint8_t bc_info);
 /* , */
 /*   uint16_t endOfPacket=0x6fed); */
 
std::vector<uint8_t> buildABCRegisterPacket
  (PacketTypes typ, uint8_t ic, uint8_t addr,
   uint32_t data, uint16_t status);

std::vector<uint8_t> buildHCCRegisterPacket(PacketTypes tpy, uint8_t addr, uint32_t data);

}

#endif
