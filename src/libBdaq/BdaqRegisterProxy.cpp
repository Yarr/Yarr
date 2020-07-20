#include "BdaqRegisterProxy.h"

uint64_t BdaqRegisterProxy::operator=(uint64_t val) {
	write(val);
	return val;
}

BdaqRegisterProxy::operator uint64_t () const {
	return read();
}
