#ifndef YARR_STAR_PACKETS
#define YARR_STAR_PACKETS

namespace StarPackets {

enum class PacketTypes {
  PR = 1, LP = 2, ABCRegRd = 4, ABCPacketTransRegRd = 7, HCCRegRd = 8,
  ABCFullTransRegRd = 11, ABCHPR = 13, HCCHPR = 14
};

/**
  * Build physics data packet for chip data.
  *
  * @param clusters Array of words for each input channel.
  * @param typ Either PR or LP.
  * @param l0_tag 7 bits of L0tag.
  * @param bc_counter Low 8 bits of internal BCID counter.
  */
std::vector<uint8_t> buildPhysicsPacket
  (const std::vector<std::vector<uint16_t>>& clusters,
   PacketTypes typ, uint8_t l0_tag, uint8_t bc_counter);

/**
  * Build physics data packet with raw cluster words.
  *
  * @param clusters Sequence of cluster words.
  * @param typ Either PR or LP.
  * @param l0_tag 7 bits of L0tag.
  * @param bc_counter Low 8 bits of internal BCID counter.
  */
std::vector<uint8_t> buildPhysicsPacket
  (const std::vector<uint16_t>& clusters,
   PacketTypes typ, uint8_t l0_tag, uint8_t bc_counter);

std::vector<uint8_t> buildABCRegisterPacket
  (PacketTypes typ, uint8_t ic, uint8_t addr,
   uint32_t data, uint16_t status);

std::vector<uint8_t> buildHCCRegisterPacket(PacketTypes tpy, uint8_t addr, uint32_t data);

}

#endif
