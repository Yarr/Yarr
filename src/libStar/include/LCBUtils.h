#ifndef STAR_LCB_HEADER
#define STAR_LCB_HEADER

#include <tuple>

namespace SixEight {

  constexpr int count_bits(uint8_t d) {
    return d == 0?0:((d&1) + count_bits((d>>1) & 0x1f));
  }

  constexpr int disparity(uint8_t d) {
    return (2*count_bits(d))-6;
  }

  inline uint8_t encode(uint8_t data6) {
    int d = disparity(data6);
    switch(d) {
    case  0: return 0x80 | data6; // Prepend 10
    case  2:
      if(data6 == 0x0f) return 0x4b;
      return 0x00 | data6; // Prepend 00
    case -2:
      if(data6 == 0x30) return 0x74;
      return 0xc0 | data6; // Prepend 11
    case -6: return 0x59; // Only 000000
    case  6: return 0x66; // Only 111111
    case -4:
      switch(data6) {
      case 0x01: return 0x71;
      case 0x02: return 0x72;
      case 0x04: return 0x65;
      case 0x08: return 0x69;
      case 0x10: return 0x53;
      case 0x20: return 0x63;
      }
    case 4:
      switch(data6) {
      case 0x3e: return 0x4e;
      case 0x3d: return 0x4d;
      case 0x3b: return 0x5a;
      case 0x37: return 0x56;
      case 0x2f: return 0x6c;
      case 0x1f: return 0x5c;
      }
    }
    return 0xff;
  }

  inline uint8_t kcode(int k) {
    switch(k) {
    case 0: case 56: return 0x78;
    case 1: case 21: return 0x55;
    case 2: case  7: return 0x47;
    case 3: case 42: return 0x6a;
    }
    return 0xff;
  }

  inline uint8_t decode(uint8_t data8) {
    switch(data8) {
      // K chars
    case 0x78: return 0x80;
    case 0x55: return 0x81;
    case 0x47: return 0x82;
    case 0x6a: return 0x83;

      // Disp 4
    case 0x4e: return 0x3e;
    case 0x4d: return 0x3d;
    case 0x5a: return 0x3b;
    case 0x56: return 0x37;
    case 0x6c: return 0x2f;
    case 0x5c: return 0x1f;

      // Disp -4
    case 0x71: return 0x01;
    case 0x72: return 0x02;
    case 0x65: return 0x04;
    case 0x69: return 0x08;
    case 0x53: return 0x10;
    case 0x63: return 0x20;

      // Disp 6/-6
    case 0x59: return 0x00;
    case 0x66: return 0x3f;

      // Single 2s
    case 0x4b: return 0x0f;
    case 0x74: return 0x30;

    default:
      // Assume not invalid!
      return data8 & 0x3f;
    }
    return 0xff;
  }

  inline bool is_valid(uint8_t data8) {
    switch(data8) {
      // K chars
    case 0x78: case 0x55: case 0x47: case 0x6a:
      return true;

      // Disp 4
    case 0x4e: case 0x4d:
    case 0x5a: case 0x56:
    case 0x6c: case 0x5c:
      return true;

      // Disp -4
    case 0x71: case 0x72:
    case 0x65: case 0x69:
    case 0x53: case 0x63:
      return true;

      // Disp 6/-6
    case 0x59: case 0x66:
      return true;

      // Single 2s
    case 0x4b: case 0x74:
      return true;

      // Others
    case 23: case 27: case 29: case 30:
    case 39: case 43: case 45: case 46:
    case 51: case 53: case 54: case 57:
    case 58: case 60: case 135: case 139:
    case 141: case 142: case 147: case 149:
    case 150: case 153: case 154: case 156:
    case 163: case 165: case 166: case 169:
    case 170: case 172: case 177: case 178:
    case 180: case 184: case 195: case 197:
    case 198: case 201: case 202: case 204:
    case 209: case 210: case 212: case 216:
    case 225: case 226: case 228: case 232:
      return true;
    }
    return false;
  }

  inline bool is_kcode(uint8_t data8) {
    return data8 == kcode(0) || data8 == kcode(1)
        || data8 == kcode(2) || data8 == kcode(3);
  }
} // Close namespace SixEight

namespace LCB {

  typedef uint16_t Frame;

  const uint8_t K0 = SixEight::kcode(0);
  const uint8_t K1 = SixEight::kcode(1);
  const uint8_t K2 = SixEight::kcode(2);
  const uint8_t K3 = SixEight::kcode(3);

  enum FastCmdType {
    NONE = 0,
    RESVD = 1,
    LOGIC_RESET = 2,
    ABC_REG_RESET = 3,
    ABC_SEU_RESET = 4,
    ABC_CAL_PULSE = 5,
    ABC_DIGITAL_PULSE = 6,
    ABC_HIT_COUNT_RESET = 7,
    ABC_HIT_COUNT_START = 8,
    ABC_HIT_COUNT_STOP = 9,
    ABC_SLOW_COMMAND_RESET = 10,
    HCC_STOP_PRLP = 11,
    HCC_REG_RESET = 12,
    HCC_SEU_RESET = 13,
    HCC_PLL_RESET = 14,
    HCC_START_PRLP = 15
  };


  constexpr Frame build_pair(uint8_t f, uint8_t s) {
    return (f << 8) | s;
  }

  inline std::tuple<uint8_t, uint8_t> split_pair(Frame f) {
    return {(f>>8)&0xff, f&0xff};
  }

  /// Idle frame
  const Frame IDLE = build_pair(K0, K1);

  inline Frame raw_bits(uint16_t bits) {
    return (SixEight::encode((bits>>6) & 0x3f) << 8) | SixEight::encode(bits&0x3f);
  }

  /// L0A: tag is 0-6, mask is 8-11
  inline Frame l0a_mask(uint8_t mask, uint8_t tag, bool bcr = false) {
    return raw_bits((bcr?0x800:0) | ((mask&0xf) << 7) | (tag & 0x7f));
  }

  /// BCR with no L0A
  inline Frame lonely_bcr() {
    return raw_bits(0x800);
  }

  /// Is this a frame with L0A and/or BCR.
  inline bool is_l0a_bcr(Frame f) {
    if(SixEight::is_kcode(f&0xff)
       || SixEight::is_kcode((f>>8) & 0xff)) return false;
    uint16_t twelve = SixEight::decode(f&0xff)
                   | (SixEight::decode((f>>8)&0xff) << 6);
    return (twelve & 0xf80) != 0;
  }

  /// Is this a frame with BCR.
  inline bool is_bcr(Frame f) {
    if(SixEight::is_kcode(f&0xff)
       || SixEight::is_kcode((f>>8) & 0xff)) return false;
    uint16_t twelve = SixEight::decode(f&0xff)
                   | (SixEight::decode((f>>8)&0xff) << 6);
    return (twelve & 0x800) != 0;
  }

  /// Extract L0 mask.
  inline int get_l0_mask(Frame f) {
    uint16_t twelve = SixEight::decode(f&0xff)
                   | (SixEight::decode((f>>8)&0xff) << 6);
    return (twelve >> 7) & 0xf;
  }

  /// Extract L0Tag.
  inline int get_l0_tag(Frame f) {
    uint16_t twelve = SixEight::decode(f&0xff)
                   | (SixEight::decode((f>>8)&0xff) << 6);
    return twelve & 0x7f;
  }

  /// Data for command (register r/w)
  inline Frame command_bits(uint16_t data) {
    return raw_bits(data & 0x7f);
  }

  /// Extract command data (register r/w)
  inline uint16_t get_command_bits(Frame f) {
    uint16_t twelve = SixEight::decode(f&0xff)
                   | (SixEight::decode((f>>8)&0xff) << 6);
    return twelve & 0x7f;
  }

  /// Resets etc. (2 bits of BC select and 4 bits of command)
  inline Frame fast_command(FastCmdType type, uint8_t delay) {
    return (LCB::K3 << 8) | SixEight::encode(((delay&3) << 4) | ((int)type & 0xf));
  }

  /// Is this a fast command frame.
  inline bool is_fast_command(Frame f) {
    return (f >> 8) == LCB::K3;
  }

  /// Get BC slot for fast command.
  inline int get_fast_command_bc(Frame f) {
    uint8_t six = SixEight::decode(f&0xff);
    return (six >> 4) & 0xf;
  }

  /// Get BC slot for fast command.
  inline FastCmdType get_fast_command(Frame f) {
    uint8_t six = SixEight::decode(f&0xff);
    return FastCmdType(six & 0xf);
  }

  /// Is valid frame (including all K codes)
  inline bool is_valid(Frame f) {
    return SixEight::is_valid(f&0xff)
      && SixEight::is_valid((f>>8) & 0xff);
  }

} // Close namespace LCB

#endif
