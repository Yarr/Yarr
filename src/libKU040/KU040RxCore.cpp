#include "KU040RxCore.h"
#include "KU040Registers.h"
#include <cstring>
//#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>

#define VLEN    30
#define BUFSIZE 256

#ifdef __APPLE__
struct mmsghdr {
    struct msghdr msg_hdr;  /* Message header */
    unsigned int  msg_len;  /* Number of received bytes for header */
};
#endif

KU040RxCore::KU040RxCore()
{
	m_enableMask = 0;
	m_skipRecsWithErrors = false;
}

KU040RxCore::~KU040RxCore()
{
	std::cout << "desctructor" << std::endl;

	// kill UDP receiver
	if(m_UDPReceiveThread != nullptr)
        {
        	m_UDPReceiveThreadRunning = false;              // kill thread
                m_UDPReceiveThread->join();
                delete m_UDPReceiveThread;
                m_UDPReceiveThread = nullptr;
        }

	// disable all channels
	for(int ch = 0; ch < 20; ch++)
	{
		std::cout << "Disabling RX channel " << ch << std::endl;
		m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0);
	}
}

void KU040RxCore::setRxEnable(uint32_t val)
{
	// check mask (if we are in 320 mode only even channels can be enabled)
	if((m_linkSpeed == 320) && ((val & 0xAAAAA) != 0))
	{
		std::cerr << "In 320 Mbit/s mode you can only select even channels. Skipping enabling/readout of odd channels!" << std::endl;
		val = val & 0x55555;
	}

	// save mask locally
	m_enableMask = val;

	// loop through channels and enable them
	for(int ch = 0; ch < 20; ch++)
	{
		// clear the FIFO and reset the counters
		m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0x18);
		m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0x0);

		if(m_enableMask & (1 << ch))
		{
			unsigned int ctrlreg;

			// enable
			ctrlreg = 0x4;

			// set channel speed
			switch(m_linkSpeed)
			{
				case 80:	ctrlreg |= 0x1; break;
				case 160:	ctrlreg |= 0x2; break;
				case 320:	ctrlreg |= 0x3; break;
				default:	ctrlreg |= 0x2; break;
			}

			// set output path
			if(m_useUDP)
			{
				// enable UDP FIFO
				ctrlreg |= 0x100;
			}
			else
			{
				// enable regbank FIFO
				ctrlreg |= 0x80;
			}

			// finally write control register
			m_com->Write(KU040_PIXEL_RX_CONTROL(ch), ctrlreg);
		}
		else
		{
			// channel is disabled
			m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0x0);
		}
	}

	// do we need to en-/disable the UDP thread
	if((val == 0) || (m_useUDP == false))
	{
		if(m_UDPReceiveThread != nullptr)
		{
			m_UDPReceiveThreadRunning = false;		// kill thread
			m_UDPReceiveThread->join();
			delete m_UDPReceiveThread;
			m_UDPReceiveThread = nullptr;
		}
	}
	else
	{
		m_UDPReceiveThreadRunning = true;
		m_UDPReceiveThread = new std::thread(&KU040RxCore::UDPReceiveThreadProc, this);
	}
}

void KU040RxCore::maskRxEnable(uint32_t val, uint32_t mask)
{
	uint32_t tmp = m_enableMask;
    tmp &= ~mask;
    val |= tmp;
    setRxEnable(val);
}

RawData* KU040RxCore::readData()
{
	std::vector<uint32_t> formatted_data;

	if(m_useUDP) {
		m_queuemutex.lock();
		if(m_queue.size() > 0) {
			for(unsigned int i = 0; i < m_queue.size(); i++) {
				formatted_data.push_back(m_queue.front());
				m_queue.pop();
			}
		}
		m_queuemutex.unlock();
	} else {
		// loop through all enabled channels and fetch the FIFO data
		for(int ch = 0; ch < 20; ch++)
		{
			// skip disabled channels
			if((m_enableMask & (1 << ch)) == 0) continue;

			// get FIFO content length
			uint32_t fifo_words = m_com->Read(KU040_PIXEL_RX_FIFOCNT(ch));

			// process if data is available
			if(fifo_words > 0)
			{
				uint32_t *buf = new uint32_t[fifo_words];

				// read everything
				m_com->Read(KU040_PIXEL_RX_FIFO(ch), buf, fifo_words, true);

				// push it
				for(uint32_t i = 0; i < fifo_words; i++)
				{
					// push only valid records or all records if m_skipRecsWithError is false
					if(!m_skipRecsWithErrors || ((buf[i] & (1U<<25)) == 0)) 
					{
						formatted_data.push_back((ch << 26) | (0x0 << 24) | (buf[i] & 0xFFFFFF));
					}
					else
					{
						std::cout << "Skipping record 0x" << std::hex << buf[i] << std::dec << std::endl;
					}
				}

				delete[] buf;
			}
		}
	}

	// return the data to caller
	if(formatted_data.size() == 0)
	{
		return NULL;
	}
	else
	{
		uint32_t *buf = new uint32_t[formatted_data.size()];
		std::copy(formatted_data.begin(), formatted_data.end(), buf);
		//std::cout << "returning " << formatted_data.size() << " records." << std::endl;
		return new RawData(0x0, buf, formatted_data.size());
	}
}

uint32_t KU040RxCore::getDataRate()
{
	return 0;
}

uint32_t KU040RxCore::getCurCount()
{
	return 0;
}

bool KU040RxCore::isBridgeEmpty()
{
	for(int ch = 0; ch < 20; ch++)
	{
		// skip disabled channels
		if((m_enableMask & (1 << ch)) == 0) continue;

		// channel is disabled
		if(m_com->Read(KU040_PIXEL_RX_STATUS(ch)) & 0x4)
		{
			return false;
		}
	}

	// if we reach this point no channel had data
	return true;
}

void KU040RxCore::setEmu(uint32_t mask, uint8_t hitcnt)
{
	// debug
	std::cout << "Setting emulator mask 0x" << std::hex << mask << std::dec << " with hitcnt=" << hitcnt << std::endl;

	// save mask
	m_emuMask = mask;

	// set emulator enable mask
	m_com->Write(KU040_PIXEL_DEBUG_EMU_ENABLE, mask);
	m_com->Write(KU040_PIXEL_DEBUG_EMU_HITDIST, 0x050003);
}

uint32_t KU040RxCore::getEmu()
{
	return m_emuMask;
}

void KU040RxCore::UDPReceiveThreadProc()
{
	// header of thread
	std::cout << "Starting UDP receive" << std::endl;

	// create a socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0)
	{
		std::cout << "Could not create DGRAM socket (error " << errno << ")" << std::endl;
		return;
	}

	// bind to local port 13330
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(13330);
	if(bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		std::cout << "Could not bind to port 13330 (error " << errno << ")" << std::endl;
		close(sockfd);
		return;
	}

	// set UDP receive timeout
	struct timespec timeout_socket;
        timeout_socket.tv_sec = 1;
        timeout_socket.tv_nsec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_socket, sizeof(timeout_socket));

	// prepare buffers
#ifndef __APPLE__
	struct mmsghdr msgs[VLEN];
	struct iovec iovecs[VLEN];
        unsigned char bufs[VLEN][BUFSIZE+1];
	memset(msgs, 0, sizeof(msgs));
	for(int j = 0; j < VLEN; j++)
	{
		iovecs[j].iov_base = bufs[j];
		iovecs[j].iov_len = BUFSIZE;
		msgs[j].msg_hdr.msg_iov = &iovecs[j];
		msgs[j].msg_hdr.msg_iovlen = 1;
	}

	unsigned int packets = 0;
	unsigned int packets_size = 0;
	while(m_UDPReceiveThreadRunning)
	{
        std::vector<uint32_t> records;
        
#ifndef __APPLE__ //TODO recvmmsg not defined on osx, possible workaround: https://github.com/eQu1NoX/trinity-osx/tree/master/syscalls
		int n = recvmmsg(sockfd, msgs, VLEN, 0, NULL);
		if(n > 0)
		{
			packets = packets + n;
			for(int i = 0; i < n; i++)
			{
				packets_size = packets_size + msgs[i].msg_len;
				for(unsigned int j = 0; j < msgs[i].msg_len; j = j + 8)
				{
					uint32_t record1 = 0;
					uint32_t record2 = 0;
					record1 |= (uint32_t)bufs[i][j] << 0;
					record1 |= (uint32_t)bufs[i][j+1] << 8;
					record1 |= (uint32_t)bufs[i][j+2] << 16;
					record1 |= (uint32_t)bufs[i][j+3] << 24;
					record2 |= (uint32_t)bufs[i][j+4] << 0;
                                        record2 |= (uint32_t)bufs[i][j+5] << 8;
                                        record2 |= (uint32_t)bufs[i][j+6] << 16;
                                        record2 |= (uint32_t)bufs[i][j+7] << 24;
					if(record2 != 0xe0f0e0f0)	records.push_back(record2);
					if(record1 != 0xe0f0e0f0)       records.push_back(record1);
				}
			}

			m_queuemutex.lock();
//			std::cout << "pushing " << records.size() << " records." << std::endl;
			for(unsigned int i = 0; i < records.size(); i++)
			{
				m_queue.push(records[i]);
			}
			m_queuemutex.unlock();
		}
#endif
	}

	std::cout << "Stopping UDP receive" << std::endl;
	std::cout << "Received " << packets << " UDP packets with total size " << packets_size << std::endl;
#endif
}

