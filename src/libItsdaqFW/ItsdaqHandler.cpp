#include "ItsdaqHandler.h"

#include <iostream>
#include <memory>
#include <thread>

#include <arpa/inet.h>

#include "Checksum.h"
#include "UdpSocket.h"

/// Private implementation (keep details out of header file)
class ItsdaqPrivate {
public:
  ItsdaqPrivate(uint32_t remote_IP,
                uint16_t srcPort, uint16_t dstPort)
    : sock(remote_IP, srcPort, dstPort)
  {
  }

  UdpSocket sock;
  std::thread receiver;
  std::atomic<bool> running;

  void ReceiverMain();
};

ItsdaqHandler::ItsdaqHandler(uint32_t remote_IP,
                             uint16_t srcPort, uint16_t dstPort) :
  priv(new ItsdaqPrivate(remote_IP, srcPort, dstPort))
{
  priv->receiver = std::thread( [&] () { priv->ReceiverMain(); });
}

ItsdaqHandler::~ItsdaqHandler() {
  std::cout << "   Join receiver thread...\n";
  priv->running = false;
  priv->receiver.join();
  std::cout << "   ... done\n";
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
}

void ItsdaqPrivate::ReceiverMain() {
  int packet_count = 0;
  int bytes_count = 0;

  while(running) {
    std::array<char, 1500> buffer;
    size_t output_bytes = 0;
    int timeout = 10;
    bool success = sock.receive(buffer, output_bytes, timeout);
    if(success) {
      packet_count ++;
      bytes_count += output_bytes;
    }
  }

  std::cout << "End ReceiverMain after receiving " << packet_count << " packets\n";
}
