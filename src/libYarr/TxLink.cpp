// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: YARR FW Library
// # Comment: Transmitter Fabric
// ################################

#include "TxLink.h"

TxLink::TxLink(TxCore *arg_core, unsigned arg_channel) {
    core = arg_core;
    channel = arg_channel;
}

TxLink::~TxLink() {

}

void TxLink::write(uint32_t value) {
    core->writeFifo(value, channel);
}
