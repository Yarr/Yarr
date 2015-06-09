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
}

// Delete all leftover data, Bookkeeper should be deleted last
Bookkeeper::~Bookkeeper() {

}

void Bookkeeper::addFe(unsigned channel, unsigned chipId) {
    feList.push_back(new Fei4(tx, chipId, channel));
	
}

int Bookkeeper::removeFe(unsigned arg_channel) {
	for(unsigned int k=0; k<feList.size(); k++) {
		if(feList[k]->getChannel() == arg_channel) {
			delete feList[k];
			feList.erase(feList.begin() + k);
			return arg_channel;
		}
	}
	return -1;
}

int Bookkeeper::removeFe(Fei4* fe) {
	unsigned arg_channel = fe->getChannel();
	return removeFe(arg_channel);
}



/*
int Bookkeeper::prepareMap() {
	configFeList.clear();
	unsigned int numberActiveFes = 0;


	
	for(unsigned int j=0; j<feList.size(); j++) {
		if(feList[j].second->getActive() == true) {
			configFeList.push_back(&feList[j]);
			numberActiveFes += 1;
		}
	}
	for(unsigned int k=0; k<configFeList.size(); k++) {
	this->eventMap.emplace(k,&configFeList[k]->second->clipDataFei4);
	}
	return numberActiveFes;
}

*/


uint32_t Bookkeeper::setFeActive(Fei4 *fe) {
	fe->setActive(true);
	uint32_t data = 0;
	data = 1 << fe->getChannel();
	activeMask |= data;
	return activeMask;
}

uint32_t Bookkeeper::setFeInactive(Fei4 *fe) {
	fe->setActive(false);
	uint32_t data = 0;
	data = 1 << fe->getChannel();
	activeMask &= !data;
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




