#ifndef HSIO_UDP_SOCKET_H
#define HSIO_UDP_SOCKET_H

#include <array>
#include <vector>

/// Wrap UDP socket to send recieve ITSDAQ format packets
class UdpSocket {
  // the socket
  int sock_fd;

  int sourcePort;
  int destinationPort;

  std::vector<char> dst_addr;

  bool shutting_down;

  void setup(uint32_t dest_addr, int srcPort, int destPort);

 public:
  static const size_t MAX_PACKET_SIZE = 1500;

  /// Open UDP socket sending to destination
  UdpSocket(uint32_t dest_addr, uint16_t srcPort, uint16_t destPort);

  /// Close socket
  ~UdpSocket();

  void send(const char *packet_start, const char *packet_end);

  bool receive(std::array<char, MAX_PACKET_SIZE> &packet, size_t &output_length, int timeout);

  void interrupt() { shutting_down = true; }
};

#endif
