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
    : sock(remote_IP, srcPort, dstPort)
  {
  }

  ~ItsdaqPrivate();

  UdpSocket sock;
  std::thread receiver;
  std::atomic<bool> running;

  ClipBoard<RawData> rawData;

  std::unique_ptr<RawData> GetData();
  void QueueData(uint16_t *start, size_t len);
  void ReceiverMain();
};

ItsdaqHandler::ItsdaqHandler(uint32_t remote_IP,
                             uint16_t srcPort, uint16_t dstPort) :
  priv(new ItsdaqPrivate(remote_IP, srcPort, dstPort))
{
  priv->receiver = std::thread( [&] () { priv->ReceiverMain(); });
}

ItsdaqHandler::~ItsdaqHandler() {
  priv.reset();
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

ItsdaqPrivate::~ItsdaqPrivate() {
  // Finish thread
  logger->debug("Join receiver thread...");
  running = false;
  receiver.join();
  logger->debug(" ...done");

  logger->debug("Flush receiver queue...");
  // Delete anything not read
  while(!rawData.empty()) {
    auto data = rawData.popData();
    delete [] data->buf;
  }
  logger->debug(" ...done");
}

void ItsdaqPrivate::QueueData(uint16_t *start, size_t len) {
  // Where to put stream number?
  // int stream = start[0];
  // int source = start[1];
  // int config = start[2];

  auto get64 = [&](size_t i) {
    uint64_t word = (uint64_t(start[i*4+3]) << 48ULL)
                  | (uint64_t(start[i*4+4]) << 32ULL)
                  | (uint64_t(start[i*4+5]) << 16ULL)
                  | (uint64_t(start[i*4+6]) << 0ULL);
    return word;
  };

  uint64_t first = get64(0);
  uint16_t stream = ntohs(start[0]);

  logger->debug("First word: {:x}", first);

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
      // Good data, wait for end
      continue;
    }

    // This word should be first TS after data
    // Copy data to queue (startOffset is last TS before data)
    startOffset ++;
    size_t len64 = i-startOffset;
    size_t len32 = len64 * 2;
    uint32_t *buf = new uint32_t[len32];
    for(int o=0; o<len64; o++) {
      auto word = get64(o+startOffset);
      buf[o*2] = (word >> 32) & 0xffffffff;
      buf[o*2+1] = word & 0xffffffff;
    }
    rawData.pushData(std::make_unique<RawData>(stream, buf, len32));
    logger->debug("QueueData: {} words ({} to {})", len64, startOffset, i);

    startOffset = i;
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

      if((opcode == 0x50) || (opcode == 0x10) || (opcode == 0x78)) {
        // Ignore acks
        continue;
      }

      logger->debug("Received opcode {:x}", opcode);

      for(size_t b_o = 0; b_o < output_bytes/2; b_o ++ ) {
        buf16[b_o] = htons(buf16[b_o]);
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

      // Don't pass CRC word
      QueueData(buf16 + offset, (output_bytes/2) - (offset+1));

      logger->debug("Queued opcode data {:x} {}", buf16[4], opcode);
    }
  }

  logger->debug("End ReceiverMain after receiving {} packets with {} bytes in {} loops",
                packet_count, bytes_count, loop_count);
}
