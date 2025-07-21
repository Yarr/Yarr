#include "FelixDirectMode.h"

#include "Bookkeeper.h"
#include "FelixTools.h"
#include "FelixTxCore.h"
#include "TxCore.h"

#include "AllStdActions.h"

#include <iomanip>

#include "logging.h"

// Borrow the spdlog fmt lib in lieu of C++20
#include "spdlog/fmt/fmt.h"

namespace {
    auto logger = logging::make_log("FelixDirectMode");
}

namespace FelixLoopsRegistry {

using StdDict::registerLoopAction;

bool direct_loop_registered = registerLoopAction
  ("FelixDirectMode",
   []() { return std::unique_ptr<LoopActionBase>(new FelixDirectMode); });

}

FelixDirectMode::FelixDirectMode()
  : LoopActionBase(LOOP_STYLE_NOP)
{
    min = 0;
    max = 1;
    step = 1;

    loopType = typeid(this);
}

void FelixDirectMode::init() {
    SPDLOG_LOGGER_DEBUG(logger, "Init");

    m_done = false;

    while(g_tx->isCmdEmpty() == 0);
}

void FelixDirectMode::end() {
    SPDLOG_LOGGER_DEBUG(logger, "End");
    while(g_tx->isCmdEmpty() == 0);
}

void FelixDirectMode::setMode(int mask, int length)
{
  // Assuming FELIX hardware and valid firmware
  g_tx->writeFwRegister("DIRECT_MODE_CTRL_START_SOURCE_MASK", mask);
  g_tx->writeFwRegister("DIRECT_MODE_CTRL_CAPTURE_LENGTH", length);
}

void FelixDirectMode::setLinksEnable(int enable)
{
  auto *ftx = dynamic_cast<FelixTxCore*>(g_tx);
  if(!ftx) {
    logger->error("FelixDirectMode: not using FelixClient hardware controller");
    return;
  }

  for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
    auto &fe = keeper->getEntry(id);

    auto rx_chan = fe.rxChannel;

    auto fid = ftx->fid_from_channel(rx_chan);
    auto elink = FelixTools::elink_from_chn(rx_chan);
    auto egroup = FelixTools::egroup_from_elink(elink);

    // Use formatter from spdlog
    std::string reg_name = fmt::format("DECODING_LINK{:02}_EGROUP{}_CTRL_PATH_ENCODING",
                                       elink, egroup);

    g_tx->writeFwRegister(reg_name, 0);
  }
}

void FelixDirectMode::execPart1() {
    SPDLOG_LOGGER_DEBUG(logger, "-> Set direct mode");

    // Only one bin
    g_stat->set(this, 0);

    setMode(m_start_source_mask, m_capture_length);

    setLinksEnable(1);

    while(g_tx->isCmdEmpty() == 0) {
        // Not sure reg uses the same Tx
        logger->trace("Waiting for cmd to empty");
    }
}

void FelixDirectMode::execPart2() {
    SPDLOG_LOGGER_DEBUG(logger, " End Direct mode");

    setMode(0, 0);

    setLinksEnable(0);

    while(g_tx->isCmdEmpty() == 0) {
        // Not sure reg uses the same Tx
        logger->trace("Waiting for cmd to empty");
    }

    m_done = true;
}

void FelixDirectMode::writeConfig(json &config) {
    config["captureLength"] = m_capture_length;
    config["startSourceMask"] = m_start_source_mask;
}

void FelixDirectMode::loadConfig(const json &config) {
    if(config.contains("captureLength")) {
        m_capture_length = config["captureLength"];
    }

    m_start_source_mask = config["startSourceMask"];

    logger->debug("Loaded FelixDirectMode configuration length {} and source mask {:08b}",
                  m_capture_length, m_start_source_mask);

    static const std::string bit_names[] = {
      "!Idle", "Idle", "SOP", "EOP", "Low", "High", "ByteLow", "ByteHigh",
      "", "", "", "", "", "", "", "None"
    };

    std::vector<std::string> bits;

    for(unsigned i=0; i<16; i++) {
      if(m_start_source_mask & (1<<i)) {
        bits.push_back(bit_names[i]);
      }
    }

    std::string bit_string;

    for(size_t i=0; i<bits.size(); i++) {
      if(!bit_string.empty()) {
        bit_string += "|";
      }
      bit_string += bits[i];
    }

    logger->debug(" source mask bits {}", bit_string);

    if(m_capture_length > 15) {
        logger->warn("Capture length too big (0-15)");
    }
}
