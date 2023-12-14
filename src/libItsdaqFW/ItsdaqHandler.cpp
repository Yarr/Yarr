#include "ItsdaqHandler.h"

#include <iostream>
#include <memory>
#include <thread>

#include <arpa/inet.h>

#include "Checksum.h"
#include "UdpSocket.h"
#include "Utils.h"
#include "logging.h"

namespace {
auto logger = logging::make_log("ItsdaqFW::Handler");
}

/// Private implementation (keep details out of header file)
class ItsdaqPrivate {
public:
  ItsdaqPrivate(uint32_t remote_IP,
                uint16_t srcPort, uint16_t dstPort)
    : sock(remote_IP, srcPort, dstPort),
      running(true)
  {
    receiver = std::thread( [&] () { ReceiverMain(); });
    partial_buffer.clear();
  }

  ~ItsdaqPrivate();

  void reconfigure(uint32_t remote_IP, uint16_t srcPort, uint16_t dstPort) {
    sock.setup(remote_IP, srcPort, dstPort);
  }

  UdpSocket sock;

  std::unique_ptr<RawData> GetData();

  std::vector<uint16_t> latest_status_regs;
  std::vector<uint16_t> latest_sys_status_regs;

private:
  std::thread receiver;
  std::atomic<bool> running;

  ClipBoard<RawData> rawData;
  std::vector<uint64_t> partial_buffer;

  void QueueData(uint16_t *start, size_t len);
  void ReceiverMain();
};

ItsdaqHandler::ItsdaqHandler(uint32_t remote_IP,
                             uint16_t srcPort, uint16_t dstPort) :
  priv(new ItsdaqPrivate(remote_IP, srcPort, dstPort))
{
}

ItsdaqHandler::~ItsdaqHandler() {
  priv.reset();
}

void ItsdaqHandler::reconfigure(uint32_t remote, uint16_t srcPort, uint16_t dstPort) {
  priv->reconfigure(remote, srcPort, dstPort);
}

void ItsdaqHandler::SendOpcode(uint16_t opcode, uint16_t *data, uint16_t length)
{
  int extras = 8;
  std::vector<uint16_t> buffer(extras + length);

  uint16_t send_seq = 0x1234;

  buffer[0] = 0x8765;
  buffer[1] = send_seq;
  buffer[2] = (length + 8) * 2;
  buffer[3] = 1;
  buffer[4] = opcode;
  buffer[5] = ~send_seq;
  buffer[6] = length*2;

  std::copy(data, data+length, &buffer[7]);

  for(size_t b_o = 0; b_o < length + 7; b_o ++ ) {
    buffer[b_o] = htons(buffer[b_o]);
  }

  uint16_t checksum_pos = length + extras - 1;
  uint16_t checksum = calculate_checksum_range((unsigned char*)buffer.data(), (unsigned char*)(buffer.data() + checksum_pos));

  buffer[checksum_pos] = checksum;

  priv->sock.send((char*)buffer.data(), (char*)buffer.data() + (buffer.size() * 2));

  logger->debug("SendOpcode {:04x}", opcode);
}

std::unique_ptr<RawData> ItsdaqHandler::GetData() {
  return priv->GetData();
}

const std::vector<uint16_t> &ItsdaqHandler::LatestStatus() {
  return priv->latest_status_regs;
}

const std::vector<uint16_t> &ItsdaqHandler::LatestSysStatus() {
  return priv->latest_sys_status_regs;
}

ItsdaqPrivate::~ItsdaqPrivate() {
  // Finish thread
  logger->debug("Join receiver thread...");
  running = false;
  receiver.join();
  logger->debug(" ...done");

  logger->debug("Flush receiver queue...");
  // Delete anything not read
  int count = 0;
  while(!rawData.empty()) {
    auto data = rawData.popData();
    count ++;
  }
  if(count) {
    logger->debug(" ...done ({} stray data blocks)", count);
  } else {
    logger->debug(" ...done");
  }
}

void ItsdaqPrivate::QueueData(uint16_t *start, size_t len) {
  // Where to put stream number?
  // int stream = start[0];
  // int source = start[1];
  // int config = start[2];

  auto get64 = [&](size_t i) {
    if(i>=len) {
      logger->info("Oops {} {}", i, len);
      return 0UL;
    }
    uint64_t word = (uint64_t(start[i*4+3]) << 48ULL)
                  | (uint64_t(start[i*4+4]) << 32ULL)
                  | (uint64_t(start[i*4+5]) << 16ULL)
                  | (uint64_t(start[i*4+6]) << 0ULL);
    return word;
  };

  uint64_t first = get64(0);
  uint16_t stream = ntohs(start[0]);

  // logger->debug("First word: {:x} (after header {:x} {:x} {:x})",
  //               first, start[0], start[1], start[2]);

  // Three word header and two word trailer
  size_t wordCount = (len - (3+2)) / 4;

  size_t startOffset = -1;
  for(int i=0; i<wordCount; i++) {
    uint64_t thisWord = get64(i);

    if(((thisWord>>60) & 0xf) == 0xf) {
      // Timestamp
      if(startOffset == i-1) {
        startOffset = i;
        continue;
      }
    } else {
	// Good data, wait for end and
	// store good data into partial_buffer
	partial_buffer.push_back(thisWord);
	continue;
    }


    // This word should be first TS after data
    // Copy data to queue (startOffset is last TS before data)
    startOffset ++;
    size_t len64 = partial_buffer.size();
    size_t len32 = len64 * 2;
    std::unique_ptr<RawData> data(new RawData(stream, len32));
    uint32_t *buf = data->getBuf();
    buf[0] = 0;
    size_t buf_off = 0;
    bool seen_sop = false;
    for(int o=0; o<len64; o++) {
      auto word = partial_buffer.at(o);
      // 0-6 bits indicate cntrl (3c, dc, 0 are SOP, EOP, IDLE)
      uint8_t map = word >> 56;
      // logger->trace("QueueData {}: {:016x} {:07b}", o, word, map);
      for(int m=6; m>=0; m--) {
        uint8_t byte = word >> (m*8);
        if(map & (1<<m)) {
          // Control byte
          if(byte == 0xdc) {
            size_t n_words = (buf_off+3)/4;
            data->resize(n_words);
            rawData.pushData(std::move(data));
            logger->trace("QueueData: {} words (to {}) {}", n_words, i,
                          (void*)(buf));
            buf_off = 0;
            data = std::make_unique<RawData>(stream, len32);
            buf = data->getBuf();
            buf[0] = 0;
          } else if(byte == 0x3c) {
            seen_sop = true;
          } else if(!((byte == 0x3c) || (byte == 0xdc) || (byte == 0))) {
            logger->warn("QueueData {}.{}: Bad control {:02x}",
                         o, m, byte);
          }
        } else {
          if(buf_off == 0 && !seen_sop) {
            logger->warn("QueueData {}.{}: Bad data before SOP {:02x}",
                         o, m, byte);
          }

          buf[buf_off/4] |= byte << ((buf_off%4) * 8);
          // logger->trace("QueueData {}.{}: {} {:08x}", o, m,
          //               buf_off, buf[buf_off/4]);
          buf_off ++;
          if((buf_off%4) == 0) {
            buf[buf_off/4] = 0;
          }
        }
      }
    }
    if(buf_off > 0) {
      size_t n_words = (buf_off+3)/4;
      logger->warn("QueueData: Extra (no EOP) {} words ({} from {} to {}) (ptr {})", n_words, buf_off, startOffset, i, (void*)(buf));
     data->resize(n_words);
      rawData.pushData(std::move(data));
    }

    startOffset = i;
    partial_buffer.clear();
  }
}

std::unique_ptr<RawData> ItsdaqPrivate::GetData() {
  return rawData.popData();
}

void ItsdaqPrivate::ReceiverMain() {
  logger->debug("Start ReceiverMain");

  int packet_count = 0;
  int loop_count = 0;
  int bytes_count = 0;

  while(running) {
    loop_count ++;
    std::array<char, 1500> buffer;
    size_t output_bytes = 0;
    int timeout = 10;

    bool success = sock.receive(buffer, output_bytes, timeout);
    if(success) {
      packet_count ++;
      bytes_count += output_bytes;

      uint16_t *buf16 = (uint16_t*)buffer.data();

      uint16_t opcode = htons(buf16[4]);

      if((opcode == 0x50) || (opcode == 0x10) || (opcode == 0x78)
         || (opcode == 0xf2)) {
        // Ignore acks
        logger->debug("Ignore ack {:x}", opcode);
        continue;
      }

      logger->debug("Received opcode {:x}", opcode);

      for(size_t b_o = 0; b_o < output_bytes/2; b_o ++ ) {
        buf16[b_o] = ntohs(buf16[b_o]);
      }

      if(opcode == 0x19 || opcode == 0x15 || opcode == 0xf9) {
        logger->debug("Report FW {} block",
                      (opcode==0x15)?"register":((opcode==0x19)?"status":"sysstat"));
        std::array<uint16_t, 4> line_data;
        for(int i=0; i<(output_bytes/2)-7; i++) {
          line_data[i%4] = buf16[i+7];
          if((i % 4) == 3)  {
            logger->debug(" {:2}: {:04x} {:04x} {:04x} {:04x}", i-3,
                          line_data[0], line_data[1],
                          line_data[2], line_data[3]);
          }
        }
        if(opcode == 0x19) {
          std::vector<uint16_t> status_regs;

          for(int i=0; i<(output_bytes/2)-7; i++) {
            status_regs.push_back(buf16[i+7]);
          }

          latest_status_regs = status_regs;
        }
        if(opcode == 0xf9) {
          std::vector<uint16_t> status_regs;

          for(int i=0; i<(output_bytes/2)-7; i++) {
            status_regs.push_back(buf16[i+7]);
          }

          latest_sys_status_regs = status_regs;
        }

        continue;
      }

#if 0
      for(int i=0; i<output_bytes/2; i++) {
        std::cout << " " << Utils::hexify(buf16[i]);
        if((opcode & 0xf000) == 0xd000) {
          if(i<10) continue;
          if(((i-10) % 4) < 3) continue;
        }
        std::cout << "\n";
      }
#endif

      // Strip off header (including opcode number)
      int offset = 7;

      if(output_bytes < offset * 2) {
        logger->debug("Data too small to be queued ({})", output_bytes);
        continue;
      }

      if((opcode & 0xf000) != 0xd000) {
        logger->info("Ignore unknown opcode {:x}", opcode);
        continue;
      }

      // Don't pass CRC word
      QueueData(buf16 + offset, (output_bytes/2) - (offset+1));

      logger->debug("Queued opcode data {:x} {}", buf16[4], opcode);
    }
  }

  logger->debug("End ReceiverMain after receiving {} packets with {} bytes in {} loops",
                packet_count, bytes_count, loop_count);
}
