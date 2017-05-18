#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "IPbus.h"

IPbus::IPbus()
{
	m_socketfd = -1;
}

IPbus::IPbus(const std::string &host, const unsigned int port)
{
	m_socketfd = -1;
	Connect(host, port);
}

IPbus::~IPbus()
{
	// close socket if it is opened
	if(m_socketfd >= 0)
	{
		close(m_socketfd);
	}
}

void IPbus::Connect(const std::string &host, const unsigned int port)
{
	// create datagram UDP socket
	m_socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(m_socketfd < 0)
	{
		throw(IPbusException("Could not create UDP socket."));
	}

	// resolve host
	struct hostent *h = gethostbyname(host.c_str());
	if(h == NULL)
	{
		throw(IPbusException("Could not resolve host '" + host + "'."));
	}
	
	// save the first entry in the list as remote host
	memset(&m_server, 0, sizeof(m_server));
	m_server.sin_family = h->h_addrtype;
	memcpy(&m_server.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
	m_server.sin_port = htons(port);

	// free up space from gethostbyname
	//free(h);

	// reset transaction and packet id
	m_packet_id = 0;
	m_transaction_id = 0;

	// set receive timeout to 1000 ms
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(m_socketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
}

void IPbus::Disconnect()
{
	// close socket
	if(m_socketfd >= 0)
	{
		close(m_socketfd);
	}

	// reset remote addr
	memset(&m_server, 0, sizeof(m_server));
}

bool IPbus::IsConnected()
{
	return (m_socketfd >= 0);
}

uint32_t IPbus::Read(uint32_t baseaddr)
{
	uint32_t ret;

	Read(baseaddr, &ret, 1);

	return ret;
}

ssize_t IPbus::Read(uint32_t baseaddr, uint32_t *buffer, size_t words, bool non_inc)
{
	// check if buffer is valid
	if(buffer == NULL)
	{
		throw(IPbusException("Supplied buffer is invalid in."));
	}

	// if number of words is larger than 255 we need to split the transaction
	if(words > 255)
	{
		uint32_t baseaddr_a = baseaddr;
		uint32_t baseaddr_b = (non_inc ? baseaddr : (baseaddr + 255));
		ssize_t tr_a = Read(baseaddr_a, &buffer[0], 255, non_inc);
		ssize_t tr_b = Read(baseaddr_b, &buffer[255], words-255, non_inc);
		return tr_a + tr_b;
	}

	// allocate buffers for RX and TX
	uint32_t *tx = new uint32_t[3];
	uint32_t *rx = new uint32_t[words + 2];

	// prepare TX packet
	tx[0] = (0x2 << 28) | (0xf << 4);
	tx[1] = (0x2 << 28) | ((m_transaction_id & 0xFFF) << 16) | ((words & 0xFF) << 8) | 0xF;
	tx[2] = baseaddr;
	if(non_inc)
	{
		tx[1] |= 0x2 << 4;
	}

	// send out packet
	if(sendto(m_socketfd, tx, 3*sizeof(uint32_t), 0, (struct sockaddr *) &m_server, sizeof(m_server)) < 0)
	{
		throw(IPbusException("Could not send read request."));
	}

	// receive response
	struct sockaddr_in src;
	socklen_t srclen = sizeof(struct sockaddr_in);
	int len = recvfrom(m_socketfd, rx, (words+2) * sizeof(uint32_t), 0, (struct sockaddr *)&src, &srclen);

	// check response
	if(len < 0)
	{
		std::cout << "error " << errno << ": " << strerror(errno) << std::endl;
		throw(IPbusException("Receive from failed with error " + std::to_string((long long int)errno) + " in Read."));
	}
	else
	{
		if(len != (int)((words+2) * sizeof(uint32_t)))
		{
			throw(IPbusException("Receive response is too small."));
		}
		else
		{
			for(int i = 0; i < (ssize_t)words; i++)
			{
				buffer[i] = rx[i+2];
			}
		}
	}

	// increment transaction id
	m_transaction_id++;

	// delete buffers
	delete[] rx;
	delete[] tx;

	// return
	return words;
}

void IPbus::Write(uint32_t baseaddr, uint32_t value)
{
	Write(baseaddr, &value, 1);
}

ssize_t IPbus::Write(uint32_t baseaddr, uint32_t *buffer, size_t words, bool non_inc)
{
	// check if buffer is valid
	if(buffer == NULL)
	{
		throw(IPbusException("Supplied buffer is invalid."));
	}

	// if number of words is larger than 255 we need to split the transaction
	if(words > 255)
	{
		uint32_t baseaddr_a = baseaddr;
		uint32_t baseaddr_b = (non_inc ? baseaddr : (baseaddr + 255));
		ssize_t tr_a = Write(baseaddr_a, &buffer[0], 255, non_inc);
		ssize_t tr_b = Write(baseaddr_b, &buffer[255], words-255, non_inc);
		return tr_a + tr_b;
	}

	// allocate buffers for RX and TX
	uint32_t *tx = new uint32_t[3 + words];
	uint32_t *rx = new uint32_t[1];

	// prepare TX packet
	tx[0] = (0x2 << 28) | (0xf << 4);
	tx[1] = (0x2 << 28) | ((m_transaction_id & 0xFFF) << 16) | ((words & 0xFF) << 8) | (0x1 << 4) | 0xF;
	tx[2] = baseaddr;
	if(non_inc)
	{
		tx[1] |= 0x2 << 4;
	}
	for(int i = 0; i < (ssize_t)words; i++)
	{
		tx[3+i] = buffer[i];
	}

	// send out packet
	if(sendto(m_socketfd, tx, (3+words)*sizeof(uint32_t), 0, (struct sockaddr *) &m_server, sizeof(m_server)) < 0)
	{
		throw(IPbusException("Could not send read request."));
	}

	// receive response
	struct sockaddr_in src;
	socklen_t srclen = sizeof(struct sockaddr_in);
	int len = recvfrom(m_socketfd, rx, sizeof(uint32_t), 0, (struct sockaddr *)&src, &srclen);

	// check response
	if(len < 0)
	{
		std::cout << "error " << errno << ": " << strerror(errno) << std::endl;
		throw(IPbusException("Receive from failed with error " + std::to_string((long long int)errno) + " in Write."));
	}
	else
	{
		if(len != sizeof(uint32_t))
		{
			throw(IPbusException("Receive response is too small."));
		}
	}

	// increment transaction id
	m_transaction_id++;

	// delete buffers
	delete[] rx;
	delete[] tx;

	// return
	return words;
}

void IPbus::RMWbits(uint32_t baseaddr, uint32_t and_term, uint32_t or_term)
{
	// allocate buffers for RX and TX
	uint32_t *tx = new uint32_t[5];
	uint32_t *rx = new uint32_t[2];

	// prepare TX packet
	tx[0] = (0x2 << 28) | (0xf << 4);
	tx[1] = (0x2 << 28) | ((m_transaction_id & 0xFFF) << 16) | (0x1 << 8) | (0x4 << 4) | 0xF;
	tx[2] = baseaddr;
	tx[3] = and_term;
	tx[4] = or_term;
	

	// send out packet
	if(sendto(m_socketfd, tx, 5*sizeof(uint32_t), 0, (struct sockaddr *) &m_server, sizeof(m_server)) < 0)
	{
		throw(IPbusException("Could not send read request."));
	}

	// receive response
	struct sockaddr_in src;
	socklen_t srclen = sizeof(struct sockaddr_in);
	int len = recvfrom(m_socketfd, rx, 2*sizeof(uint32_t), 0, (struct sockaddr *)&src, &srclen);

	// check response
	if(len < 0)
	{
		std::cout << "error " << errno << ": " << strerror(errno) << std::endl;
		throw(IPbusException("Receive from failed with error " + std::to_string((long long int)errno) + " in Write."));
	}
	else
	{
		if(len != 2*sizeof(uint32_t))
		{
			throw(IPbusException("Receive response is too small."));
		}
	}

	// increment transaction id
	m_transaction_id++;

	// delete buffers
	delete[] rx;
	delete[] tx;
}

void IPbus::RMWsum(uint32_t baseaddr, uint32_t addend)
{
	// allocate buffers for RX and TX
	uint32_t *tx = new uint32_t[4];
	uint32_t *rx = new uint32_t[2];

	// prepare TX packet
	tx[0] = (0x2 << 28) | (0xf << 4);
	tx[1] = (0x2 << 28) | ((m_transaction_id & 0xFFF) << 16) | (0x1 << 8) | (0x5 << 4) | 0xF;
	tx[2] = baseaddr;
	tx[3] = addend;

	// send out packet
	if(sendto(m_socketfd, tx, 4*sizeof(uint32_t), 0, (struct sockaddr *) &m_server, sizeof(m_server)) < 0)
	{
		throw(IPbusException("Could not send read request."));
	}

	// receive response
	struct sockaddr_in src;
	socklen_t srclen = sizeof(struct sockaddr_in);
	int len = recvfrom(m_socketfd, rx, 2*sizeof(uint32_t), 0, (struct sockaddr *)&src, &srclen);

	// check response
	if(len < 0)
	{
		std::cout << "error " << errno << ": " << strerror(errno) << std::endl;
		throw(IPbusException("Receive from failed with error " + std::to_string((long long int)errno) + " in Write."));
	}
	else
	{
		if(len != 2*sizeof(uint32_t))
		{
			throw(IPbusException("Receive response is too small."));
		}
	}

	// increment transaction id
	m_transaction_id++;

	// delete buffers
	delete[] rx;
	delete[] tx;
}

