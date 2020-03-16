#ifndef BDAQFIFOREADOUT_H
#define BDAQFIFOREADOUT_H

#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "BdaqSiTcpFifo.h"


class BdaqFifoReadout {
	public:
		BdaqFifoReadout(BdaqSiTcpFifo& _fifo) : fifo(_fifo) {std::cout << "BdaqFifoReadout!\n";}

		~BdaqFifoReadout() {
			//stopThread();
		}

		void init();
		
		/*void reset();
		std::size_t getAvailableWords();
		std::vector<uint32_t> getData();*/

	protected:
		BdaqSiTcpFifo& fifo;
		
		/*std::thread readThread;
		bool runThread = false;
		std::vector<uint32_t> data; //MOVE OUTTA HERE!? 

		void startThread();
		void stopThread();	
		void readData();*/

};

#endif
