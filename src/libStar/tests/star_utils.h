#ifndef YARR_STAR_TEST_UTILS
#define YARR_STAR_TEST_UTILS

struct RegExtractInfo {
  /// The id of the register write packet.
  uint8_t reg;

  /// The register value written.
  uint32_t value;

  /// Other info in the packet (eg communication IDs)
  uint32_t other;

  /// Count of idle words
  size_t idle{0};

  /// Count of non-idle words
  size_t count{0};

  /// Count of trigger/reset frames
  size_t count_lb{0};

  /// Count of fast command frames
  size_t count_fast{0};

  bool isRegWrite() const {
    return count_lb == 0 && count_fast == 0 && count == 9;
  }

  bool isHccRegWrite() const {
    return isRegWrite() && (other & 0x20) == 0;
  }

  bool isAbcRegWrite() const {
    return isRegWrite() && (other & 0x20) == 0x20;
  }
};

/**
   Override TxCore to record what is written to FIFO.
 */
class CapturePacketsTxCore : public EmptyTxCore {
public:
  std::vector<std::vector<uint32_t>> buffers;

  void writeFifo(uint32_t word) override {
    if(buffers.empty()) {
      buffers.push_back(std::vector<uint32_t>{});
    }
    buffers.back().push_back(word);
  }

  void releaseFifo() override {
    buffers.push_back(std::vector<uint32_t>{});
  }

  /// Get LCB frame from sent words, index is in time order 
  LCB::Frame getFrame(int buff_id, size_t idx) const {
    REQUIRE (buff_id < buffers.size());

    const auto &buffer = buffers[buff_id];

    REQUIRE (idx < buffer.size() * 2);
    size_t offset = idx/2;
    // High word is first in time
    int side = (idx+1)%2;
    return (buffer[offset] >> (16*side)) & 0xffff;
  }

  /**
   * Check recorded data for register value.
   *
   * @param buff_id Record number to read (separated by releaseFifo)
   * @return Extracted packet info (mostly register).
   */
  RegExtractInfo getRegValueForBuffer(int buff_id) const {
    RegExtractInfo ri{};
    ri.value = 0;

    int progress = 0;
    for(int i=0; i<buffers[buff_id].size()*2; i++) {
      LCB::Frame f = getFrame(buff_id, i);
      CAPTURE (buff_id, i, f, progress);
      if(f == LCB::IDLE) {
        ri.idle ++;
        continue;
      }

      ri.count ++;

      // Nothing beyond end
      REQUIRE (progress < 9);

      uint8_t code0 = (f >> 8) & 0xff;
      uint8_t code1 = f & 0xff;

      if(code0 == LCB::K3) {
          // Fast command
          ri.count_fast++;
          continue;
      }

      if(code0 == LCB::K2) {
        if(progress == 0) {
          // Start (ignore flags)
          // ABC/HCC + HCC ID
          ri.other &= 0xffffff00;
          ri.other |= SixEight::decode(code1);
          progress ++;
          continue;
        } else if(progress == 8) {
          // End (ignore flags)
          ri.other &= 0x00ffffff;
          ri.other |= SixEight::decode(code1) << 24;
          progress ++;
          continue;
        }
      }

      REQUIRE (!SixEight::is_kcode(code1));
      REQUIRE (!SixEight::is_kcode(code0));

      uint16_t data12 = (SixEight::decode(code0) << 6)
                       | SixEight::decode(code1);

      if((data12 & (~0x7f))) {
        // L0A/BCR
        ri.count_lb++;
        continue;
      }

      if(progress == 1) {
        ri.other &= 0xfff00fff;
        ri.other |= (data12&0xffc) << 10;

        ri.reg &= 0x3f;
        ri.reg |= (data12&3)<<6;
      } else if(progress == 2) {
        ri.reg &= 0xc0;
        ri.reg |= (data12>>1) & 0x3f;
        // value |= (data12 & 0x7f) << (7*(7-progress)));
      } else {
        CAPTURE(data12);
        REQUIRE( progress != 0 );
        ri.value |= (data12 & 0x7f) << (7*(7-progress));
        CAPTURE(ri.value);
        // CHECK (progress == 2) ;
      }

      progress ++;
    }

    return ri;
  }
};

#endif
