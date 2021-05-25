#include "UdpSocket.h"

#include <iostream>

#include "sys/types.h"
#include "sys/socket.h"
#include "netdb.h"
#include "unistd.h"

#include "logging.h"

namespace {
auto logger = logging::make_log("ItsdaqFW::UDP");
}

/*
 * Example network setup on Linux (lost on reboot, for testing):
 *
 *  ifconfig eth1 192.168.222.100 netmask 255.255.255.0
 */

UdpSocket::UdpSocket(uint32_t remote, uint16_t srcPort, uint16_t dstPort)
  : shutting_down(false)  
{
  setup(remote, srcPort, dstPort);
} 

UdpSocket::~UdpSocket()
{
  // Maybe check not in recieve loop?
  close(sock_fd);
}

void UdpSocket::setup(uint32_t remote, int srcPort, int dstPort)
{
  sourcePort = srcPort;
  destinationPort = dstPort;

  logger->debug("Creating UDP socket to IP {:08x} ports: {} to {}", remote, srcPort, dstPort);

  // Look up destination address
  uint32_t addr_ip4 = remote;

  logger->debug("IP address: {}.{}.{}.{}",
                (addr_ip4>>0)&0xff, (addr_ip4>>8)&0xff,
                (addr_ip4>>16)&0xff, (addr_ip4>>24)&0xff);

  // Don't send packets on general network
  bool ip_address_ok = false;
  if(addr_ip4 == 0x0100007f) ip_address_ok = true;
  else if(((addr_ip4 & 0xff) == 192) && (((addr_ip4>>8) & 0xff) == 168)) ip_address_ok = true;

  if(!ip_address_ok) {
    logger->critical("*** Validation of {}.{}.{}.{} failed, address outside 192.168.*.* and 127.0.0.1 not allowed",
                     (addr_ip4>>0)&0xff, (addr_ip4>>8)&0xff,
                     (addr_ip4>>16)&0xff, (addr_ip4>>24)&0xff);
    exit(-1);
  }

  // ie IPv4
  dst_addr.resize(sizeof(sockaddr_in));

  sockaddr_in &addr = *(sockaddr_in*)&dst_addr[0];

  addr.sin_family = AF_INET;
  addr.sin_port = htons(dstPort);
  addr.sin_addr.s_addr = addr_ip4;

  // See udp(7)
  if( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror("udp socket: ");
    throw std::runtime_error("Failed to open udp socket");
  }

  logger->trace("Have UDP socket handle: {}", sock_fd);

  // Bind says which port to receive on
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(sourcePort);

  // Could also specify device
  sin.sin_addr.s_addr = htonl(INADDR_ANY); // Don't care
  // sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // For testing

  int bindResult;

  if( (bindResult=bind(sock_fd, (struct sockaddr *)&sin, sizeof(sin))) < 0 ) {
    perror("udp_open: bind failed");
    throw std::runtime_error("Failed to bind udp socket");
  }

  int enable = 1;
  int rslt = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if(rslt != 0) {
    perror("Failed to set SO_REUSEADDR");
  }

  logger->trace("Bound socket handle: {}", sock_fd);
}

void UdpSocket::send(const char *packet_start, const char *packet_end)
{
  std::array<iovec, 1> pacvec;

  // At the moment, the HSIO packet is the whole datagram

  pacvec[0].iov_base = (void*)packet_start;
  pacvec[0].iov_len = packet_end - packet_start;

  struct msghdr msgInfo;
  msgInfo.msg_name = &dst_addr[0];
  msgInfo.msg_namelen = dst_addr.size();
  msgInfo.msg_iov = &pacvec[0];
  msgInfo.msg_iovlen = pacvec.size();
  msgInfo.msg_control = 0;
  msgInfo.msg_controllen = 0;
  msgInfo.msg_flags = 0;

  int result = sendmsg(sock_fd, &msgInfo,0);
  if(result == -1) {
    perror("Send to udpSocket failed");
    throw std::runtime_error("Failed to write to udp socket");
  }
}

bool UdpSocket::receive(std::array<char, 1500> &buffer, size_t &output_bytes, int timeout)
{
  logger->trace("UdpSocket::receive");

  // Currently ignore timeout parameter (could use select, or fcntl? to set read timeout)
  int timeoutCounter = 0;

  const int timeout_ms_step = 100;

  while(1) {
    timeoutCounter += timeout_ms_step;

    fd_set selList;
    FD_ZERO(&selList);
    FD_SET(sock_fd, &selList);

    timeval timeOutVal;
    // Always have a timeout, so we can shutdown
    timeOutVal.tv_sec = 0;
    timeOutVal.tv_usec = 1000 * timeout_ms_step;

    int result = select(sock_fd + 1, &selList, NULL, NULL, &timeOutVal);

    if(shutting_down) {
      std::cerr << "UdpSocket shutting down from receive\n";
      return false;
    }

    if(result == -1) {
      int recordErrno = errno;

      if(recordErrno == EINTR) continue;

      std::cerr << "Select failed: " << strerror(recordErrno) << " (" << recordErrno << ") " << std::endl;
      throw std::runtime_error("Select call failed, waiting for input from file");
    }

    if(result != 0) {
      // Something ready
      break;
    }

    if(timeoutCounter > timeout) {
      // Not an error because there might be idle times
      logger->debug("UdpSocket timeout waiting for packet");
      return false;
    }
  }

  int result = recv(sock_fd, &buffer[0], MAX_PACKET_SIZE, 0);

  if(result == -1) {
    perror("Receive fom udpSocket failed:");
    throw std::runtime_error("Failed to receive from udp socket");
  }

  // std::copy(buffer, buffer + result, &packet[0]);
  output_bytes = result;

  logger->trace("Received {} bytes from UDP", result);

  return true;
}
