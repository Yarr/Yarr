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
    g_fe = new Fei4(tx, 8); // Broadcast to all
    g_fe65p2 = new Fe65p2(tx); // Hardware only allows single FE65-P2
    target_tot = 10;
    target_charge = 16000;
    target_threshold = 3000;
}

// Delete all leftover data, Bookkeeper should be deleted last
Bookkeeper::~Bookkeeper() {
    for(unsigned int k=0; k<feList.size(); k++) {
        delete feList[k];
        feList.erase(feList.begin() + k);
    }
    delete g_fe;
    delete g_fe65p2;
}

// RxChannel is unique ident
void Bookkeeper::addFe(FrontEnd *fe, unsigned txChannel, unsigned rxChannel) {
    if(isChannelUsed(rxChannel)) {
        std::cerr << __PRETTY_FUNCTION__ << " -> Error rx channel already in use, not adding FE" << std::endl;
    } else {
        feList.push_back(fe);
        dynamic_cast<FrontEndCfg*>(feList.back())->setChannel(txChannel, rxChannel);
        eventMap[rxChannel];
        histoMap[rxChannel];
        resultMap[rxChannel];
        feList.back()->clipDataFei4 = &eventMap[rxChannel];
        feList.back()->clipHisto = &histoMap[rxChannel];
        feList.back()->clipResult = &resultMap[rxChannel];
        mutexMap[rxChannel];
    }
    std::cout << __PRETTY_FUNCTION__ << " -> Added FE: Tx(" << txChannel << "), Rx(" << rxChannel << ")" << std::endl;
}

void Bookkeeper::addFe(FrontEnd *fe, unsigned channel) {
    this->addFe(fe, channel, channel);
}

// RxChannel is unique ident
void Bookkeeper::delFe(unsigned rxChannel) {
    if (!isChannelUsed(rxChannel)) {
        std::cerr << __PRETTY_FUNCTION__ << " -> Error rx channel not in use, can not delete FE" << std::endl;
    } else {
        for(unsigned int k=0; k<feList.size(); k++) {
            if(dynamic_cast<FrontEndCfg*>(feList[k])->getChannel() == rxChannel) {
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

void Bookkeeper::delFe(FrontEnd* fe) {
    unsigned arg_channel = dynamic_cast<FrontEndCfg*>(fe)->getChannel();
    delFe(arg_channel);
}

FrontEnd* Bookkeeper::getFeByChannel(unsigned arg_channel) {
    for(unsigned int k=0; k<feList.size(); k++) {
        if(dynamic_cast<FrontEndCfg*>(feList[k])->getChannel() == arg_channel) {
            return feList[k];
        }
    }
    return NULL;	
}

FrontEnd* Bookkeeper::getFe(unsigned rxChannel) {
    for(unsigned int k=0; k<feList.size(); k++) {
        if(dynamic_cast<FrontEndCfg*>(feList[k])->getChannel() == rxChannel) {
            return feList[k];
        }
    }
    return NULL;
}

FrontEnd* Bookkeeper::getLastFe() {
    return feList.back();
}

bool Bookkeeper::isChannelUsed(unsigned arg_channel) {
    for (unsigned i=0; i<feList.size(); i++) {
        if (dynamic_cast<FrontEndCfg*>(feList[i])->getRxChannel() == arg_channel)
            return true;
    }
    return false;
}

uint32_t Bookkeeper::getTxMask() {
    uint32_t mask = 0;
    for (unsigned i=0; i<feList.size(); i++) {
        if (feList[i]->isActive()) {
            mask |= (0x1 << dynamic_cast<FrontEndCfg*>(feList[i])->getTxChannel());
        }
    }
    return mask;
}

uint32_t Bookkeeper::getRxMask() {
    uint32_t mask = 0;
    for (unsigned i=0; i<feList.size(); i++) {
        if (feList[i]->isActive()) {
            mask |= (0x1 << dynamic_cast<FrontEndCfg*>(feList[i])->getRxChannel());
        }
    }
    return mask;
}
