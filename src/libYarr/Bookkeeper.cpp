// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include "Bookkeeper.h"

Bookkeeper::Bookkeeper(TxCore *arg_tx, RxCore *arg_rx) {
	tx = arg_tx;
	rx = arg_rx;
	g_fe = new Fei4(tx, 0);
	usedChannels = 0x0;
}

// Delete all leftover data, Bookkeeper should be deleted last
Bookkeeper::~Bookkeeper() {

}

void Bookkeeper::addFe(unsigned channel, unsigned chipId) {
	if(isChannelUsed(channel)) {
		std::cout << "No FE added on channel #" << channel << " with chipId " << chipId << std::endl;
		return;
	}
	usedChannels |= (1<< channel);
    feList.push_back(new Fei4(tx, chipId, channel));
	mutexMap.insert(std::pair<unsigned, std::mutex*>(channel,new std::mutex));
	prepareMap();
}

int Bookkeeper::removeFe(unsigned arg_channel) {
		// This way is valid if channel numbers are unique!
	for(unsigned int k=0; k<feList.size(); k++) {
		if(feList[k]->getChannel() == arg_channel) {
			delete feList[k];

				usedChannels &= !(1 << arg_channel);

			feList.erase(feList.begin() + k);
			mutexMap.erase(arg_channel);
			prepareMap();
			return arg_channel;
		}
	}
	return -1;
}

int Bookkeeper::removeFe(Fei4* fe) {
	unsigned arg_channel = fe->getChannel();
	prepareMap();
	return removeFe(arg_channel);
}

Fei4* Bookkeeper::getFei4byChannel(unsigned arg_channel) {
	for(unsigned int k=0; k<feList.size(); k++) {
		if(feList[k]->getChannel() == arg_channel) {
			return feList[k];
		}
	}
	return NULL;	
}

int Bookkeeper::prepareMap() {
	unsigned int n = 0;
	activeFeList.clear();
	for(unsigned int j=0; j<feList.size(); j++) {
		if(feList[j]->getActive() == true) {
			activeFeList.push_back(feList[j]);
			this->eventMap.emplace(n,&feList[j]->clipDataFei4);
			n += 1;
		}
	}
	return n;
}

bool Bookkeeper::isChannelUsed(unsigned arg_channel) {
	uint32_t ch = 1 << arg_channel;
	if(ch & usedChannels) {
		std::cout << "Error: channel #" << arg_channel << " already assigned to a FE!" << std::endl;
		return true;
	}
	else
		return false;
}

uint32_t Bookkeeper::setFeActive(Fei4 *fe) {
	fe->setActive(true);
	uint32_t data = 0;
	data = 1 << fe->getChannel();
	activeMask |= data;
	this->prepareMap();
	return activeMask;
}

uint32_t Bookkeeper::setFeInactive(Fei4 *fe) {
	fe->setActive(false);
	uint32_t data = 0;
	data = 1 << fe->getChannel();
	activeMask &= !data;
	this->prepareMap();
	return activeMask;
}

uint32_t Bookkeeper::collectActiveMask() {
	uint32_t data = 0;
	uint32_t newMask = 0;
	for(unsigned int k=0; k<feList.size(); k++) {
		if(feList[k]->getActive()) {
			data = 1 << feList[k]->getChannel();
			newMask |= data;		
		}
	}
	activeMask = newMask;
	return activeMask;
}

int Bookkeeper::dummy() {
	return 667;
}

