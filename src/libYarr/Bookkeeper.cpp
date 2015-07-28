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
    g_fe = new Fei4(tx, 8);
    target_tot = 10;
    target_charge = 16000;
    target_threshold = 3000;
}

// Delete all leftover data, Bookkeeper should be deleted last
Bookkeeper::~Bookkeeper() {
    delete g_fe;
}

// RxChannel is unique ident
void Bookkeeper::addFe(unsigned chipId, unsigned txChannel, unsigned rxChannel) {
    if(isChannelUsed(rxChannel)) {
        std::cerr << __PRETTY_FUNCTION__ << " -> Error rx channel already in use, not adding FE" << std::endl;
    } else {
        feList.push_back(new Fei4(tx, chipId, txChannel, rxChannel));
        eventMap[rxChannel];
        histoMap[rxChannel];
        resultMap[rxChannel];
        feList.back()->clipDataFei4 = &eventMap[rxChannel];
        feList.back()->clipHisto = &histoMap[rxChannel];
        feList.back()->clipResult = &resultMap[rxChannel];
        mutexMap[rxChannel];
    }
    std::cout << __PRETTY_FUNCTION__ << " -> Added FE: ChipId(" << chipId << "), Tx(" << txChannel << "), Rx(" << rxChannel << ")" << std::endl;
}

void Bookkeeper::addFe(unsigned chipId, unsigned channel) {
    this->addFe(chipId, channel, channel);
}

// RxChannel is unique ident
void Bookkeeper::delFe(unsigned rxChannel) {
    if (!isChannelUsed(rxChannel)) {
        std::cerr << __PRETTY_FUNCTION__ << " -> Error rx channel not in use, can not delete FE" << std::endl;
    } else {
        for(unsigned int k=0; k<feList.size(); k++) {
            if(feList[k]->getChannel() == rxChannel) {
                delete feList[k];
                feList.erase(feList.begin() + k);
            }
        }
        mutexMap.erase(rxChannel);
        histoMap.erase(rxChannel);
        resultMap.erase(rxChannel);
        mutexMap.erase(rxChannel);
    }
}

void Bookkeeper::delFe(Fei4* fe) {
    unsigned arg_channel = fe->getChannel();
    delFe(arg_channel);
}

Fei4* Bookkeeper::getFei4byChannel(unsigned arg_channel) {
    for(unsigned int k=0; k<feList.size(); k++) {
        if(feList[k]->getChannel() == arg_channel) {
            return feList[k];
        }
    }
    return NULL;	
}

Fei4* Bookkeeper::getFe(unsigned rxChannel) {
    for(unsigned int k=0; k<feList.size(); k++) {
        if(feList[k]->getChannel() == rxChannel) {
            return feList[k];
        }
    }
    return NULL;
}

Fei4* Bookkeeper::getLastFe() {
    return feList.back();
}

bool Bookkeeper::isChannelUsed(unsigned arg_channel) {
    for (unsigned i=0; i<feList.size(); i++) {
        if (feList[i]->getRxChannel() == arg_channel)
            return true;
    }
    return false;
}

uint32_t Bookkeeper::getTxMask() {
    uint32_t mask = 0;
    for (unsigned i=0; i<feList.size(); i++) {
        if (feList[i]->isActive()) {
            mask |= (0x1 << feList[i]->getTxChannel());
        }
    }
    return mask;
}

uint32_t Bookkeeper::getRxMask() {
    uint32_t mask = 0;
    for (unsigned i=0; i<feList.size(); i++) {
        if (feList[i]->isActive()) {
            mask |= (0x1 << feList[i]->getRxChannel());
        }
    }
    return mask;
}
