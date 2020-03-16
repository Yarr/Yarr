// Readout tasks
// Based on bdaq53/fifo_readout.py

#include "BdaqFifoReadout.h"
#include "BdaqSiTcpFifo.h"

void BdaqFifoReadout::init() {
	//data.clear();
	//startThread();
}

//==============================================================================
//==============================================================================

/*
void BdaqFifoReadout::reset() {
	//Read TCP data until FPGA buffer gets empty... So, it looks very strange...
	stopThread();
	data.clear();
	startThread();
}


void BdaqFifoReadout::startThread() {
	runThread=true;
	readThread = std::thread(&BdaqFifoReadout::readData, this);
}

void BdaqFifoReadout::stopThread() {
	runThread=false;
	if(readThread.joinable()) {
		readThread.join();
	}
}
*/

//==============================================================================
//==============================================================================

