#include "BdaqSiTcpFifo.h"

//Returns the number of available 32-bit words for readout
std::size_t BdaqSiTcpFifo::getAvailableWords() {
	std::size_t size = tcp.getSize(); //in bytes
	return (size - (size % 4)) / 4; 
}

std::size_t BdaqSiTcpFifo::getTcpSize() {
	return tcp.getSize();
}

void BdaqSiTcpFifo::flushBuffer() {
	std::size_t size = tcp.getSize(); 
	if (size != 0) {
		std::vector<uint8_t> buf(size);
		tcp.read(buf);
	}
}

std::size_t BdaqSiTcpFifo::readData(uint32_t* buffer) {
	std::size_t wCount = getAvailableWords();
	std::vector<uint8_t> buf(wCount * 4);
        buf.clear();
	tcp.read(buf);
	for (uint i=0;i<wCount;++i) {
		buffer[i] = buf.at(i*4+0)       | buf.at(i*4+1) << 8 | 
					buf.at(i*4+2) << 16 | buf.at(i*4+3) << 24;
	}
	return wCount;
}

void BdaqSiTcpFifo::readData(std::vector<uint32_t>& buffer, std::size_t size) {
	buffer.reserve(size);
	std::vector<uint8_t> buf(size * 4);
	tcp.read(buf);
	uint32_t temp;
	for (uint i=0;i<size;++i) {
		temp      = buf.at(i*4+0)       | buf.at(i*4+1) << 8 | 
					buf.at(i*4+2) << 16 | buf.at(i*4+3) << 24;
		buffer.push_back(temp);
	}
}

