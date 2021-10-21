// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include "FrontEnd.h"

bool FrontEnd::isActive() {
	return active;
}

bool FrontEnd::getActive() {
	return this->active;
}

void FrontEnd::setActive(bool arg_active) {
	active = arg_active;
}
