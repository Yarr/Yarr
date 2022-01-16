// #################################
// # Author: Rafael Gama
// # Email: rafael.gama at cern.ch
// # Project: Yarr
// # Description: SiTCP FIFO (readout) interface
// # Comment: Based on basil/HL/sitcp_fifo.py
// ################################

#ifndef BDAQSITCPFIFO_H
#define BDAQSITCPFIFO_H

#include "BdaqTCP.h"

class BdaqSiTcpFifo {
	public:
		BdaqSiTcpFifo(BdaqTCP& _tcp) : tcp(_tcp) {}
		~BdaqSiTcpFifo() = default;

		//Returns the number of available 32-bit words for readout
		std::size_t getAvailableWords();
		std::size_t getTcpSize();
		void flushBuffer();
		std::size_t readData(uint32_t* buffer);
		void readData(std::vector<uint32_t>& buffer, std::size_t size);
		
	private:
		BdaqTCP& tcp;
		
};

#endif
