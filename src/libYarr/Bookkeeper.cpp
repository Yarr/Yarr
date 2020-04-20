// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include "Bookkeeper.h"

#include "logging.h"

namespace {
    auto blog = logging::make_log("Bookkeeper");
}

Bookkeeper::Bookkeeper(TxCore *arg_tx, RxCore *arg_rx) {
    tx = arg_tx;
    rx = arg_rx;
    g_fe = NULL; //Type not yet known
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
}

// RxChannel is unique ident
void Bookkeeper::addFe(FrontEnd *fe, unsigned txChannel, unsigned rxChannel) {
    if(isChannelUsed(rxChannel)) {
        SPDLOG_LOGGER_ERROR(blog, "Rx channel already in use, not adding FE");
    } else {
        feList.push_back(fe);
        FrontEndCfg *cfg = dynamic_cast<FrontEndCfg*>(feList.back());
        if(cfg) cfg->setChannel(txChannel, rxChannel);
        eventMap[rxChannel];
        histoMap[rxChannel];
        resultMap[rxChannel];
        feList.back()->clipData = &eventMap[rxChannel];
        feList.back()->clipHisto = &histoMap[rxChannel];
        feList.back()->clipResult = &resultMap[rxChannel];
    }

    // Using macro includes file/line info
    SPDLOG_LOGGER_INFO(blog, "Added FE: Tx({}), Rx({})", txChannel, rxChannel);
}

void Bookkeeper::addFe(FrontEnd *fe, unsigned channel) {
    this->addFe(fe, channel, channel);
}

// RxChannel is unique ident
void Bookkeeper::delFe(unsigned rxChannel) {
    if (!isChannelUsed(rxChannel)) {
        SPDLOG_LOGGER_ERROR(blog, "Rx channel not in use, can not delete FE!");
    } else {
        for(unsigned int k=0; k<feList.size(); k++) {
            if(dynamic_cast<FrontEndCfg*>(feList[k])->getChannel() == rxChannel) {
                delete feList[k];
                feList.erase(feList.begin() + k);
            }
        }
        histoMap.erase(rxChannel);
        resultMap.erase(rxChannel);
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

std::vector<uint32_t> Bookkeeper::getTxMask() {
    std::vector<uint32_t> activeChannels;
    for (unsigned i=0; i<feList.size(); i++) {
        if (feList[i]->isActive()) {
            activeChannels.push_back(dynamic_cast<FrontEndCfg*>(feList[i])->getTxChannel());
        }
    }
    return activeChannels;
}

std::vector<uint32_t> Bookkeeper::getRxMask() {
    std::vector<uint32_t> activeChannels;
    for (unsigned i=0; i<feList.size(); i++) {
        if (feList[i]->isActive()) {
            activeChannels.push_back(dynamic_cast<FrontEndCfg*>(feList[i])->getRxChannel());
        }
    }
    return activeChannels;
}
