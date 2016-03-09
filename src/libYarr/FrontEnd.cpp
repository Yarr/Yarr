// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include "FrontEnd.h"

void FrontEnd::setChannel(unsigned arg_channel) {
	txChannel = arg_channel;
	rxChannel = arg_channel;
}

void FrontEnd::setChannel(unsigned arg_txChannel, unsigned arg_rxChannel) {
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
}

bool FrontEnd::isActive() {
	return active;
}

bool FrontEnd::getActive() {
	return this->active;
}

void FrontEnd::setActive(bool arg_active) {
	active = arg_active;
}

unsigned FrontEnd::getChannel() {
	return rxChannel;
}

unsigned FrontEnd::getRxChannel() {
	return rxChannel;
}

unsigned FrontEnd::getTxChannel() {
	return txChannel;
}
