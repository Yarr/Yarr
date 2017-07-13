// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: RD53A Base class
// # Date: Jun 2017
// ################################

#include "Rd53a.h"

Rd53a::Rd53a(TxCore *core) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
    txChannel = 99;
    rxChannel = 99;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

Rd53a::Rd53a(TxCore *core, unsigned arg_channel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
	txChannel = arg_channel;
	rxChannel = arg_channel;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

Rd53a::Rd53a(TxCore *core, unsigned arg_txChannel, unsigned arg_rxChannel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

