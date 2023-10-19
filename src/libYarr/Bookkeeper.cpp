// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include <set>

#include "Bookkeeper.h"
#include "AllChips.h"

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
    for (unsigned i=0; i<bookEntries.size(); i++) {
        this->delFe(i);
    }
    delete g_fe;
}

void Bookkeeper::initGlobalFe(std::string chipType) {
    std::unique_ptr<FrontEnd> fe_tmp = StdDict::getFrontEnd(chipType);

    g_fe = fe_tmp->getGlobal().release();

    g_fe->makeGlobal();

    // The global FE may need to know the configuration of other FEs
    g_fe->connectBookkeeper(this);
}

void Bookkeeper::addFe(FrontEnd *fe, unsigned txChannel, unsigned rxChannel) {
    // Create new entry
    bookEntries.emplace_back();
    bookEntries.back().fe = fe;
    idMap[fe] = bookEntries.size()-1;
    bookEntries.back().active = true;
    bookEntries.back().txChannel = txChannel;
    bookEntries.back().rxChannel = rxChannel;

    FrontEndCfg *cfg = dynamic_cast<FrontEndCfg*>(fe);
    if(cfg) cfg->setChannel(txChannel, rxChannel);

    rxToIdMap[rxChannel].emplace_back(idMap[fe]);

    // Using macro includes file/line info
    SPDLOG_LOGGER_INFO(blog, "Added FE: Tx({}), Rx({}) under ID {}", txChannel, rxChannel, idMap[fe]);
}

void Bookkeeper::addFe(FrontEnd *fe, unsigned channel) {
    this->addFe(fe, channel, channel);
}

void Bookkeeper::delFe(unsigned id) {
    if (id >= bookEntries.size()) {
        SPDLOG_LOGGER_ERROR(blog, "Id not in use, can not delete FE!");
    } else {
        SPDLOG_LOGGER_DEBUG(blog, "Removed FE: Tx({}), Rx({})", bookEntries[id].txChannel, bookEntries[id].rxChannel);
        delete bookEntries[id].fe;
        unsigned rx = bookEntries[id].rxChannel;
        rxToIdMap[rx].erase(std::remove(rxToIdMap[rx].begin(), rxToIdMap[rx].end(), id), rxToIdMap[rx].end());
        bookEntries.erase(bookEntries.begin() + id);
    }
    // Remap everything
    for (unsigned i=0; i<bookEntries.size(); i++) {
        idMap[bookEntries[i].fe] = i;
    }
}

void Bookkeeper::delFe(FrontEnd* fe) {
    delFe(idMap[fe]);
}

FrontEnd* Bookkeeper::getFe(unsigned id) {
    if (id >= bookEntries.size()) {
        SPDLOG_LOGGER_ERROR(blog, "Id not in use, can not find FE!");
        return nullptr;
    } else {
        return bookEntries[id].fe;
    }
}

FrontEndCfg* Bookkeeper::getFeCfg(unsigned id) {
    if (id >= bookEntries.size()) {
        SPDLOG_LOGGER_ERROR(blog, "Id not in use, can not find FE!");
        return nullptr;
    } else {
        return dynamic_cast<FrontEndCfg*>(bookEntries[id].fe);
    }
}

FrontEnd* Bookkeeper::getLastFe() {
    return bookEntries.back().fe;
}

std::vector<uint32_t> Bookkeeper::getTxMask() {
    std::vector<uint32_t> activeChannels;
    for (BookEntry &entry : bookEntries) {
        if (entry.active)
            activeChannels.push_back(entry.txChannel);
    }
    return activeChannels;
}

std::vector<uint32_t> Bookkeeper::getRxMask() {
    std::vector<uint32_t> activeChannels;
    for (BookEntry &entry : bookEntries) {
        if (entry.active)
            activeChannels.push_back(entry.rxChannel);
    }
    return activeChannels;
}

std::vector<uint32_t> Bookkeeper::getTxMaskUnique() {
    std::set<uint32_t> uniqueChannels;
    for (BookEntry &entry : bookEntries) {
        if (entry.active) {
            uniqueChannels.insert(entry.txChannel);
        }
    }

    std::vector<uint32_t> vecChannels(uniqueChannels.begin(), uniqueChannels.end());
    return vecChannels;
}

std::vector<uint32_t> Bookkeeper::getRxMaskUnique() {
    std::set<uint32_t> uniqueChannels;
    for (BookEntry &entry : bookEntries) {
        if (entry.active) {
            uniqueChannels.insert(entry.rxChannel);
        }
    }

    std::vector<uint32_t> vecChannels(uniqueChannels.begin(), uniqueChannels.end());
    return vecChannels;
}


unsigned Bookkeeper::getId(FrontEnd *fe) {
    if (idMap.find(fe) != idMap.end()) {
        return idMap[fe];
    } else {
        SPDLOG_LOGGER_ERROR(blog, "Could not find Id for FrontEnd at 0x{:x}", fmt::ptr(fe));
    }
    return 0;
}

BookEntry& Bookkeeper::getEntry(unsigned id) {
    if (id >= bookEntries.size()) {
        SPDLOG_LOGGER_CRITICAL(blog, "Could not retrieve entry {}", id);
        exit(-1);
    }
    return bookEntries[id];
}

std::vector<unsigned>& Bookkeeper::getRxToId(unsigned rx) {
    // Construct empty if does not exist ok!
    return rxToIdMap[rx];
}
