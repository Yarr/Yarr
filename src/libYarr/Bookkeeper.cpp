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
    g_fe.reset();
    target_tot = 10;
    target_charge = 16000;
    target_threshold = 3000;
}

void Bookkeeper::initGlobalFe(const std::string& chipType) {
    std::unique_ptr<FrontEnd> fe_tmp = StdDict::getFrontEnd(chipType);

    g_fe = fe_tmp->getGlobal();

    g_fe->makeGlobal();

    // The global FE may need to know the configuration of other FEs
    g_fe->connectBookkeeper(this);
}

void Bookkeeper::addFe(std::unique_ptr<FrontEnd> fe, const FrontEndConnectivity& cfg) {
    // Create new entry
    bookEntries.emplace_back();
    bookEntries.back().fe = std::move(fe);
    unsigned uid = bookEntries.size()-1;
    idMap[bookEntries.back().fe.get()] = uid;
    bookEntries.back().active = true;
    bookEntries.back().txChannel = cfg.getTxChannel();
    bookEntries.back().rxChannel = cfg.getRxChannel();

    auto fe_cfg = dynamic_cast<FrontEndCfg*>(fe.get());
    if(fe_cfg) fe_cfg->setChannel(cfg);

    rxToIdMap[cfg.getRxChannel()].emplace_back(uid);

    // Using macro includes file/line info
    SPDLOG_LOGGER_INFO(blog, "Added FE: Tx({}), Rx({}) under ID {}", cfg.getTxChannel(), cfg.getRxChannel(), uid);
}

void Bookkeeper::addFe(std::unique_ptr<FrontEnd> fe, unsigned channel) {
    this->addFe(std::move(fe), FrontEndConnectivity(channel,channel));
}

void Bookkeeper::delFe(unsigned id) {
    if (id >= bookEntries.size()) {
        SPDLOG_LOGGER_ERROR(blog, "Id not in use, can not delete FE!");
    } else {
        SPDLOG_LOGGER_DEBUG(blog, "Removed FE: Tx({}), Rx({})", bookEntries[id].txChannel, bookEntries[id].rxChannel);
        unsigned rx = bookEntries[id].rxChannel;
        rxToIdMap[rx].erase(std::remove(rxToIdMap[rx].begin(), rxToIdMap[rx].end(), id), rxToIdMap[rx].end());
        bookEntries.erase(bookEntries.begin() + id);
    }
    // Remap everything
    for (unsigned i=0; i<bookEntries.size(); i++) {
        idMap[bookEntries[i].fe.get()] = i;
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
        return bookEntries[id].fe.get();
    }
}

FrontEndCfg* Bookkeeper::getFeCfg(unsigned id) {
    if (id >= bookEntries.size()) {
        SPDLOG_LOGGER_ERROR(blog, "Id not in use, can not find FE!");
        return nullptr;
    } else {
        return dynamic_cast<FrontEndCfg*>(bookEntries[id].fe.get());
    }
}

FrontEnd* Bookkeeper::getLastFe() {
    return bookEntries.back().fe.get();
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

void Bookkeeper::startFeClipboardMonitor() {

    // start clipboard monitoring thread
    runClipboardMonitor = true;
    clipboardMonitorThread_ptr.reset(new std::thread(&Bookkeeper::feClipboardMonitor, this));
}

void Bookkeeper::setFeClipboardMonitorRefreshTime(unsigned arg_clipboardMonitorRefreshTime) {
    clipboardMonitorRefreshTime = arg_clipboardMonitorRefreshTime;
}

void Bookkeeper::joinFeClipboardMonitor() {
    runClipboardMonitor = false;
    clipboardMonitorThread_ptr->join();
}

void Bookkeeper::addFeClipboardMonitor(unsigned arg_id, std::string arg_name) {
    clipboardMonitorFeIDs.push_back(arg_id);
    clipboardMonitorFeNames.push_back(arg_name);
}

void Bookkeeper::feClipboardMonitor() {
    if(clipboardMonitorFeIDs.size() == 0)
        return;
    for(unsigned i = 0; i < clipboardMonitorFeIDs.size(); i++)
        SPDLOG_LOGGER_INFO(blog, "[ ClipboardMonitor : {:^8} [{}] : Info ] Started clipboard monitor thread",  clipboardMonitorFeNames[i], clipboardMonitorFeIDs[i]);
    while(runClipboardMonitor) {
        for(unsigned i = 0; i < clipboardMonitorFeIDs.size(); i++) {
            SPDLOG_LOGGER_INFO(
                blog, "[ ClipboardMonitor : {:^8} [{}] : RawData  ] InCount:{:<8} OutCount:{:<8} QueueSize:{:<8}", 
                clipboardMonitorFeNames[i], clipboardMonitorFeIDs[i],
                bookEntries[clipboardMonitorFeIDs[i]].fe->clipRawData.getNumDataIn(),
                bookEntries[clipboardMonitorFeIDs[i]].fe->clipRawData.getNumDataOut(),
                bookEntries[clipboardMonitorFeIDs[i]].fe->clipRawData.size()
            );
            SPDLOG_LOGGER_INFO(
                blog, "[ ClipboardMonitor : {:^8} [{}] : ProcData ] InCount:{:<8} OutCount:{:<8} QueueSize:{:<8}", 
                clipboardMonitorFeNames[i], clipboardMonitorFeIDs[i],
                bookEntries[clipboardMonitorFeIDs[i]].fe->clipData.getNumDataIn(),
                bookEntries[clipboardMonitorFeIDs[i]].fe->clipData.getNumDataOut(),
                bookEntries[clipboardMonitorFeIDs[i]].fe->clipData.size()
            );
        }
        std::this_thread::sleep_for(std::chrono::microseconds(clipboardMonitorRefreshTime)); // microseconds  
    }
    SPDLOG_LOGGER_INFO(blog, "Joined clipboard monitor thread");
}