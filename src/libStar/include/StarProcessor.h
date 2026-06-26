#ifndef STAR_PROCESSOR_H
#define STAR_PROCESSOR_H

// Based on starChipPacket.h, but use templates to allow processing during parsing

#include <bitset>

/// Allowed response packet types
enum StarPacketType {
  SPT_PR,
  SPT_LP,
  SPT_ABC_RR,
  SPT_ABC_TRANSP,
  SPT_HCC_RR,
  SPT_ABC_FULL,
  SPT_ABC_HPR,
  SPT_HCC_HPR,
};

/**
 * Parse and process an HCCStar packet.
 *
 * Generate information for various packet types, corresponding to a single
 * HCCStar Packet.
 *
 * Inputs are begin, end of packet data (excluding SOP/EOP).
 */
template<typename StarProcessor>
bool StarProcessPacket(const uint8_t *b, const uint8_t *e, StarProcessor t)
{
  t.begin();
  size_t count = e-b;
  if( count < 4 ) {
    t.parse_error("Not enough words");
    return false;
  }

  int raw_type = (b[0]>>4) & 0xf;

  int type = (raw_type & 0xe) >> 1;

  // Map types (StarPacketType enum) to codes (ie includes parity flag)
  const std::array<uint8_t, 8> type_codes{1, 2, 4, 7, 8, 11, 13, 14};

  if( type_codes[type] != raw_type) {
    t.parse_error("Bad packet type");
    return false;
  }

  t.packet_type(type);

  if(type == SPT_HCC_RR || type == SPT_HCC_HPR) {
    if(count != 6) {
      t.parse_error("Expect size 6 for HCC read packet");
      return false;
    }
    uint8_t address = ((b[0] & 0xf) << 4) | ((b[1] >> 4) & 0xf);
    uint32_t value = ((b[1] & 0xf) << 28) | ((b[2] & 0xff) << 20) |
      ((b[3] & 0xff) << 12) | ((b[4] & 0xff) << 4) | ((b[5] >> 4) & 0xf);
    t.hcc_read(type == SPT_HCC_HPR, address, value);
  } else if (type == SPT_ABC_RR || type == SPT_ABC_HPR) {
    if(count != 9) {
      t.parse_error("Expect size 9 for ABC read packet");
      return false;
    }

    int channel_abc = b[0] & 0xf;
    uint8_t address = b[1] & 0xff;
    // Top nibble of raw_words[2] should be all 0
    uint32_t value = ((b[2] & 0xf) << 28)
      | ((b[3] & 0xff) << 20)
      | ((b[4] & 0xff) << 12)
      | ((b[5] & 0xff) << 4)
      | ((b[6] & 0xf0) >> 4);

    uint16_t abc_status = ((b[6] & 0xf) << 12)
      | ((b[7] & 0xff) << 4)
      | ((b[8] & 0xf0) >> 4);

    t.abc_read(type == SPT_ABC_HPR, channel_abc, address, value, abc_status);
  } else if(type == SPT_LP || type == SPT_PR) {
    int flag = (b[0] >> 3) & 1;
    int l0id = ((b[0] & 0b111) << 4) | ((b[1] >> 4) & 0xf);
    int bcid = (b[1] >> 1 & 0x7);
    int bcid_parity = (b[1] & 0x1);

    bool continue_parsing = true;
    unsigned int iW=3;

    t.data_header(type == SPT_PR, bcid, bcid_parity, l0id, flag);

    while( continue_parsing ) {
      if( iW+1 > count ) {
        t.parse_error("Reached end of words without the end pattern!");
        return false;
      }

      uint16_t word = (b[iW-1] << 8) | b[iW];

      if(word == 0x77f4) {
        t.begin_error_block();
        if( count < iW+8 ) {
          t.parse_error("Expect eight 8-bit error block words (including 0x77f4 marker)");
          return false;
        }

        uint64_t error_word = 0;
        for(unsigned int i=0; i < 6; ++i){
          // Word index increments (2->7), amount of shift decrements (40->0)
          error_word |= uint64_t(b[iW+1+i]) << (8*(5-i));
        }

        t.error_info(0, (error_word >> 36) & 0x7ff);
        t.error_info(1, (error_word >> 24) & 0x7ff);
        t.error_info(2, (error_word >> 12) & 0x7ff);
        t.error_info(3, (error_word >> 0) & 0x7ff);

        iW += 8;
        t.end_error_block();
      } else if(word == 0x6fed) {
        // End of physics packet
        continue_parsing = false;
      } else if((word&0x7ff) == 0x3fe) {
        // No cluster (from ABC)
        unsigned int input_channel = (word >> 11) & 0xf;
        t.data_no_cluster(input_channel);
        // Ignore this cluster
        iW += 2;
      } else {
        uint16_t raw_cluster = word & 0x7ff;

        unsigned int input_channel = (word >> 11) & 0xf;
        int address = (raw_cluster >> 3) & 0xff;
        int next = raw_cluster & 0x7;

        if(word & 0x8000) {
          t.parse_error("Bad cluster, bit 15 set");
        } else {
          t.data_raw_cluster(input_channel, raw_cluster);
          t.data_cluster(input_channel, address, next);
        }

        // Increment current word
        iW += 2;
      }
    } // while continue_parsing

    t.data_end();
  } else if(type == SPT_ABC_TRANSP) {
    // This should be a single wrapped data packet from ABC
    int channel_abc = b[0] & 0xf;

    uint64_t bits{};
    for(int i=0; i<8; i++) {
      bits <<= 8;
      bits |= b[1+i];
    }

    t.transparent_word(channel_abc, bits);
  } else {
    t.unexpected_type(type);
  }

  t.end();
  return true;
}

struct EmptyProc {
  void begin() {}
  void parse_error(const char *) {}
  void packet_type(int) {}
  void begin_error_block() {}
  void error_info(int, uint16_t) {}
  void end_error_block() {}

  void data_header(bool, uint8_t, bool, uint8_t, int) {}
  void data_no_cluster(unsigned int) {}
  void data_raw_cluster(unsigned int, uint16_t) {}
  void data_cluster(unsigned int, uint8_t, int) {}
  void data_end() {}

  void abc_read(bool, int, int, int, int) {}
  void hcc_read(bool, int, int) {}
  void transparent_word(int, uint64_t) {}
  void unexpected_type(int) {}
  void end() {}
};

/// Run Proc in sequence
template <typename... T>
struct SeqProc;

// Specialize for 1... template parameters
template <typename P, typename... T>
struct SeqProc<P, T...> {
  P pp;
  SeqProc<T...> tt;
  SeqProc(P p, T... t) : pp{p}, tt{t...} {}
  void begin() { pp.begin(); tt.begin(); }
  void parse_error(const char *msg) { pp.parse_error(msg); tt.parse_error(msg); }
  void packet_type(int t) { pp.packet_type(t); tt.packet_type(t); }

  void begin_error_block() {
    pp.begin_error_block();
    tt.begin_error_block();
  }

  void error_info(int c, uint16_t m) {
    pp.error_info(c, m);
    tt.error_info(c, m);
  }

  void end_error_block() {
    pp.end_error_block();
    tt.end_error_block();
  }

  void data_header(bool pr_not_lp, uint8_t bcid, bool parity, uint8_t l0id, int flag) {
    pp.data_header(pr_not_lp, bcid, parity, l0id, flag);
    tt.data_header(pr_not_lp, bcid, parity, l0id, flag);
  }

  void data_no_cluster(unsigned int ic) {
    pp.data_no_cluster(ic);
    tt.data_no_cluster(ic);
  }

  void data_raw_cluster(unsigned int ic, uint16_t cl_raw) {
    pp.data_raw_cluster(ic, cl_raw);
    tt.data_raw_cluster(ic, cl_raw);
  }

  void data_cluster(unsigned int ic, uint8_t addr, unsigned int hits) {
    pp.data_cluster(ic, addr, hits);
    tt.data_cluster(ic, addr, hits);
  }

  void data_end() {
    pp.data_end();
    tt.data_end();
  }

  void abc_read(bool hpr_not_rr, unsigned int ic, uint8_t address, uint32_t value, uint16_t status) {
    pp.abc_read(hpr_not_rr, ic, address, value, status);
    tt.abc_read(hpr_not_rr, ic, address, value, status);
  }
  void hcc_read(bool hpr_not_rr, uint8_t address, uint32_t value) {
    pp.hcc_read(hpr_not_rr, address, value);
    tt.hcc_read(hpr_not_rr, address, value);
  }

  void transparent_word(int ic, uint64_t word) {
    pp.transparent_word(ic, word);
    tt.transparent_word(ic, word);
  }

  void unexpected_type(int p) {
    pp.unexpected_type(p);
    tt.unexpected_type(p);
  }

  void end() {
    pp.end();
    tt.end();
  }
};

// Specialize for 0 template parameters
template <>
struct SeqProc<> : public EmptyProc {};

#endif
