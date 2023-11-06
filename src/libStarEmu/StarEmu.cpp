#include "StarEmu.h"

#include <chrono>
#include <fstream>
#include <thread>

#include "AllHwControllers.h"
#include "EmuCom.h"
#include "EmuRxCore.h"
#include "LCBUtils.h"
#include "ScanHelper.h"
#include "logging.h"

#include "StarChips.h"
#include "StarCfg.h"

namespace {
auto logger = logging::make_log("StarEmu");
}

StarEmu::StarEmu(std::vector<ClipBoard<RawData>*> &rx, EmuCom * tx, EmuCom * tx2,
                 const std::string& json_emu_file_path,
                 const std::vector<std::string>& json_chip_file_paths,
                 unsigned hpr_period, int abc_version, int hcc_version)
    : m_txRingBuffer ( tx )
    , m_txRingBuffer2 ( tx2 )
    , m_bccnt ( 0 )
    , m_resetbc ( false )
{
    run = true;

    // HCCStar and ABCStar chip configurations
    assert(rx.size()==json_chip_file_paths.size());
    logger->debug("Making star configs with A{} H{}", abc_version, hcc_version);
    for (unsigned i=0; i<json_chip_file_paths.size(); i++) {
        // Set up the chip configuration and load it to the emulator
        const std::string& regCfgFile = json_chip_file_paths[i];

        auto regCfg = std::make_unique<StarCfg>(abc_version, hcc_version);
        if (not regCfgFile.empty()) {
            json jChips;
            try {
                jChips = ScanHelper::openJsonFile(regCfgFile);
            } catch (std::runtime_error &e) {
                logger->error("Error opening chip config: {}", e.what());
                throw(std::runtime_error("StarEmu::StarEmu"));
            }
            regCfg->loadConfig(jChips);
        } else {
            // No chip configuration provided. Default: one HCCStar + one ABCStar
            regCfg->setHCCChipId(i);
            regCfg->addABCchipID(15);
        }

        chipEmus.emplace_back( new StarChipsetEmu(rx[i], json_emu_file_path, std::move(regCfg), hpr_period, abc_version, hcc_version) );
    }
}

StarEmu::~StarEmu() = default;

//
// Decode LCB
//
void StarEmu::decodeLCB(LCB::Frame frame) {
    // HPR
    this->doHPR(frame);

    // check if it is a valid frame
    if (LCB::is_valid(frame)) {
        SPDLOG_LOGGER_TRACE(logger, "Raw LCB frame = 0x{:x} BC = {}", frame, m_bccnt);
    } else {
        logger->debug("Invalid LCB frame received: 0x{:x} @ BC = {}", frame, m_bccnt);
        logger->debug("Skip decoding");
        return;
    }

    // Overwrite data that are older than L0 buffer depth
    // Fill L0 buffer with zeros and BCID for this LCB frame (4 BCs)
    this->fillL0Buffer();

    // {code0, code1}
    uint8_t code0 = (frame >> 8) & 0xff;
    uint8_t code1 = frame & 0xff;

    bool iskcode0 = SixEight::is_kcode(code0);
    bool iskcode1 = SixEight::is_kcode(code1);

    if (not (iskcode0 or iskcode1) ) {
        // Neither of the 8-bit symbol is a kcode
        // Decode the 16-bit frame to the 12-bit data
        uint16_t data12 = (SixEight::decode(code0) << 6) | SixEight::decode(code1);
        
        if ( (data12 >> 7) & 0x1f ) {
            // Top 5 bits are not zeros: has a BCR and/or triggers
            bool bcr = (data12 >> 11) & 1;  // BC reset
            uint8_t l0a_mask = (data12 >> 7) & 0xf; // 4-bit L0A mask
            uint8_t l0a_tag = data12 & 0x7f; // 7-bit L0A tag
            this->doL0A(bcr, l0a_mask, l0a_tag);
            if (bcr) m_resetbc = true;
        }
        else {
            // Top 5 bits are all zeros: part of a command sequence
            this->doRegReadWrite(frame);
        }
    }
    else {
        // Kcode detected
        if (code0 == LCB::K3) { // Fast command
            // decode the second symbol
            uint8_t k3cmd = SixEight::decode(code1);
            this->doFastCommand(k3cmd);
        }
        else if (code0 == LCB::K2) { // Start or end of a command sequence

            this->doRegReadWrite(frame);
        }
        else if (frame == LCB::IDLE) { // Idle
            SPDLOG_LOGGER_TRACE(logger, "Receive an IDLE");
            // do nothing
        }
    } // if (not (iskcode0 or iskcode1) )

    this->updateBC();
}

//
// Decode R3L1
//
void StarEmu::decodeR3L1(uint16_t frame) {
    SPDLOG_LOGGER_TRACE(logger, "Raw LCB frame = 0x{:x} BC = {}", frame, m_bccnt);

    if (frame == LCB::IDLE) { // Idle
        SPDLOG_LOGGER_TRACE(logger, "Receive an IDLE");
        // do nothing
    } else {
        // {code0, code1}
        uint8_t code0 = (frame >> 8) & 0xff;
        uint8_t code1 = frame & 0xff;

        // check if they are valid 8b code before decoding
        if ( not (SixEight::is_valid(code0) and SixEight::is_valid(code1)) ) {
            logger->debug("Invalid 8-bit code received: code0 = 0x{:x}, code1 = 0x{:x}", code0, code1);
            logger->debug("Skip decoding");
            return;
        }

        // decode the 16-bit frame to 12-bit data
        uint16_t data12 = (SixEight::decode(code0) << 6) | SixEight::decode(code1);
        // top 5 bits: mask/marker
        uint8_t mask = (data12 >> 7) & 0x1f;
        // bottom 7 bits: l0tag
        uint8_t l0tag = data12 & 0x7f;

        this->doPRLP(mask, l0tag);
    }
}

void StarEmu::executeLoop() {
    logger->info("Starting emulator loop");

    static const auto SLEEP_TIME = std::chrono::milliseconds(1);

    while (run) {
        if (m_txRingBuffer2) {
            // two tx channels
            // wait until neither of them are empty
            if (m_txRingBuffer->isEmpty() or m_txRingBuffer2->isEmpty()) {
                std::this_thread::sleep_for( SLEEP_TIME );
                continue;
            }
        } else {
            // only one tx
            if ( m_txRingBuffer->isEmpty()) {
                std::this_thread::sleep_for( SLEEP_TIME );
                continue;
            }
        }

        logger->debug("{}: -----------------------------------------------------------", __PRETTY_FUNCTION__);

        // get data
        uint16_t d0_r3l1, d1_r3l1;
        if (m_txRingBuffer2) {
            uint32_t d_r3l1 = m_txRingBuffer2->read32();
            d0_r3l1 = (d_r3l1 >> 16) & 0xffff;
            d1_r3l1 = (d_r3l1 >> 0) & 0xffff;
        }

        uint32_t d_lcb = m_txRingBuffer->read32();
        uint16_t d0_lcb = (d_lcb >> 16) & 0xffff;
        uint16_t d1_lcb = (d_lcb >> 0) & 0xffff;

        if (m_txRingBuffer2) decodeR3L1(d0_r3l1);
        decodeLCB(d0_lcb);

        if (m_txRingBuffer2) decodeR3L1(d1_r3l1);
        decodeLCB(d1_lcb);
    }
}

// Have to do this specialisation before instantiation in EmuController.h!

template<>
class EmuRxCore<StarChips> : virtual public RxCore {
        std::map<uint32_t, std::unique_ptr<ClipBoard<RawData>> > m_queues;
        std::map<uint32_t, bool> m_channels;
    public:
        EmuRxCore();
        ~EmuRxCore() override;

        EmuRxCore(const EmuRxCore &other) = delete;
        EmuRxCore(EmuRxCore &&other) = delete;
        EmuRxCore &operator =(const EmuRxCore &) = delete;
        EmuRxCore &operator =(EmuRxCore &&) = delete;
        
        void setCom(uint32_t chn, std::unique_ptr<ClipBoard<RawData>> queue);
        ClipBoard<RawData>* getCom(uint32_t chn);

        void setRxEnable(uint32_t channel) override;
        void setRxEnable(std::vector<uint32_t> channels) override;
        void maskRxEnable(uint32_t val, uint32_t mask) override {}
        void disableRx() override;
        void enableRx();
        std::vector<uint32_t> listRx();

        std::vector<RawDataPtr> readData() override;
        RawDataPtr readData(uint32_t chn);
        
        uint32_t getDataRate() override {return 0;}
        uint32_t getCurCount(uint32_t chn) {return m_queues[chn]->empty()?0:1;}
        uint32_t getCurCount() override {
            uint32_t cnt = 0;
            for (auto& q : m_queues) {
                if (m_channels[q.first])
                    cnt += EmuRxCore<StarChips>::getCurCount(q.first);
            }
            return cnt;
        }

        bool isBridgeEmpty() override {
            for (auto& q : m_queues) {
                if (m_channels[q.first])
                    if (not q.second->empty()) return false;
            }
            return true;
        }
};


#include "EmuController.h"

template<class FE, class ChipEmu>
std::unique_ptr<HwController> makeEmu() {
    auto ctrl = std::make_unique< EmuController<FE, ChipEmu> >();
    return ctrl;
}

EmuRxCore<StarChips>::EmuRxCore() = default;

EmuRxCore<StarChips>::~EmuRxCore() {
    // detele data that are not read out
    for (auto& q : m_queues) {
        while(not q.second->empty()) {
            std::unique_ptr<RawData> tmp = q.second->popData();
        }
    }
}

void EmuRxCore<StarChips>::setCom(uint32_t chn, std::unique_ptr<ClipBoard<RawData>> queue) {
    m_queues[chn] = std::move(queue);
    m_channels[chn] = true;
}

ClipBoard<RawData>* EmuRxCore<StarChips>::getCom(uint32_t chn) {
    if (m_queues.find(chn) != m_queues.end()) {
        return m_queues[chn].get();
    } else {
        return nullptr;
    }
}

RawDataPtr EmuRxCore<StarChips>::readData(uint32_t chn) {
    // //std::this_thread::sleep_for(std::chrono::microseconds(1));
    if(m_queues[chn]->empty()) return nullptr;

    std::unique_ptr<RawData> rd = m_queues[chn]->popData();
    // set rx channel number
    rd->getAdr() = chn;

    return std::move(rd);
}

std::vector<RawDataPtr> EmuRxCore<StarChips>::readData() {
    std::vector<RawDataPtr> dataVec;
   for (auto& q : m_queues) {
        if (not m_channels[q.first]) continue;
        if (q.second->empty()) continue;
        dataVec.push_back(EmuRxCore<StarChips>::readData(q.first));
    }
    return dataVec;
}

void EmuRxCore<StarChips>::setRxEnable(uint32_t channel) {
    if (m_queues.find(channel) != m_queues.end())
        m_channels[channel] = true;
    else {
        logger->critical("Channel {} has not been configured!", channel);
        logger->warn("Available rx channels:");
        for (auto& q: m_queues)
            logger->warn("    {}", q.first);
    }
}

void EmuRxCore<StarChips>::setRxEnable(std::vector<uint32_t> channels) {
    for (auto channel : channels) {
        this->setRxEnable(channel);
    }
}

void EmuRxCore<StarChips>::enableRx() {
    for (auto& q: m_queues) {
        m_channels[q.first] = true;
    }
}

void EmuRxCore<StarChips>::disableRx() {
    for (auto& q : m_queues) {
        m_channels[q.first] = false;
    }
}

std::vector<uint32_t> EmuRxCore<StarChips>::listRx() {
    std::vector<uint32_t> rxChannels;
    rxChannels.reserve(m_queues.size());
    for (auto& q: m_queues) {
        rxChannels.push_back(q.first);
    }
    return rxChannels;
}

bool emu_registered_Emu =
  StdDict::registerHwController("emu_Star",
                                makeEmu<StarChips, StarEmu>);

template<>
void EmuController<StarChips, StarEmu>::loadConfig(const json &j) {

  //TODO make nice
  logger->info("-> Starting Emulator");
  std::string emuCfgFile;
  if (j.contains("feCfg")) {
    emuCfgFile = j["feCfg"];
    logger->info("Using config: {}", emuCfgFile);
  }

  // Rx wait time
  if (j.contains("rxWaitTime")) {
      m_waitTime = std::chrono::microseconds(j["rxWaitTime"]);
  }

  // HPR packet:
  // 40000 BC (i.e. 1 ms) by default.
  // Can be set to a smaller value for testing, but need to be a multiple of 4
  unsigned hprperiod = 40000;
  if (j.contains("hprPeriod")) {
    hprperiod = j["hprPeriod"];
    logger->debug("HPR packet transmission period is set to {} BC", hprperiod);
  }

  unsigned abc_version = 0;
  unsigned hcc_version = 0;

  if (j.contains("abcVersion")) {
    abc_version = j["abcVersion"];
    logger->debug("ABC Version set to {}", abc_version);
  }

  if (j.contains("hccVersion")) {
    hcc_version = j["hccVersion"];
    logger->debug("HCC Version set to {}", hcc_version);
  }

  json chipCfg;
  if (j.contains("chipCfg")) {
    try {
      chipCfg = ScanHelper::openJsonFile(j["chipCfg"]);
    } catch (std::runtime_error &e) {
      logger->error("Error opening chip config: {}", e.what());
      throw(std::runtime_error("EmuController::loadConfig failure"));
    }
    logger->info("Using chip config: {}", std::string(j["chipCfg"]));
  } else {
    logger->info("Chip configuration is not provided. One emulated HCCStar and ABCStar chip will be generated.");
    chipCfg["chips"] = json::array();
    chipCfg["chips"][0] = {{"tx", 0}, {"rx", 1}};
  }

  // HCCStars with the same tx channel are grouped together
  // Store their rx channels and register config files in two maps
  // Key: pair of tx channel numbers (tx, tx2)
  // Value of the first map: a vector of register config file path
  // Value of the second map: a vector of pointers to rx buffers
  std::map<std::pair<int, int>, std::vector<std::string>> cfgMap;
  std::map<std::pair<int, int>, std::vector<ClipBoard<RawData>*>> rxMap;

  // StarChipsetEmu instances with the same tx channel are handled in the same StarEmu instance. Their second tx channel, if configured, must be the same as well.
  // The rx channels can be different.
  for (unsigned i=0; i<chipCfg["chips"].size(); i++) {
    // Tx
    int chn_tx = chipCfg["chips"][i]["tx"];
    // 2nd Tx for R3L1 in case of multi-level trigger mode
    int chn_tx2 = -1;
    if  (chipCfg["chips"][i].contains("tx2"))
      chn_tx2 = chipCfg["chips"][i]["tx2"];

    // Rx
    int chn_rx = chipCfg["chips"][i]["rx"];
    // Set up rx link if not already done so
    if (not EmuRxCore<StarChips>::getCom(chn_rx)) {
      EmuRxCore<StarChips>::setCom(chn_rx, std::make_unique<ClipBoard<RawData>>());
    }

    std::string regCfgFile;
    if (chipCfg["chips"][i].contains("config"))
      regCfgFile = chipCfg["chips"][i]["config"];

    auto txlabel = std::make_pair(chn_tx, chn_tx2);
    if (rxMap.find(txlabel) != rxMap.end()) {
      // This tx channel has already been set up
      // Update the existing channel setup
      cfgMap[txlabel].push_back(regCfgFile);
      rxMap[txlabel].push_back(EmuRxCore<StarChips>::getCom(chn_rx));
    } else {
      // Create a new tx channel (for a new StarEmu instance)
      //assert(not EmuTxCore<StarChips>::getCom(chn_tx));
      if (EmuTxCore<StarChips>::getCom(chn_tx)) {
        logger->error("Tx channel {} has already been set up!", chn_tx);
        throw std::runtime_error("Fail to load emulator configuration");
      }
      tx_coms.emplace_back(new RingBuffer());
      EmuTxCore<StarChips>::setCom(chn_tx, tx_coms.back().get());

      // TX2 if required
      if (chn_tx2 >= 0) {
        //assert(not EmuTxCore<StarChips>::getCom(chn_tx2));
        if (EmuTxCore<StarChips>::getCom(chn_tx2)) {
          logger->error("Tx channel {} has already been set up!", chn_tx2);
          throw std::runtime_error("Fail to load emulator configuration");
        }
        tx_coms.emplace_back(new RingBuffer());
        EmuTxCore<StarChips>::setCom(chn_tx2, tx_coms.back().get());
      }

      cfgMap[txlabel] = {regCfgFile};
      rxMap[txlabel] = {EmuRxCore<StarChips>::getCom(chn_rx)};
    }

  } // end of chipCfg["chips"] loop

  // Configure and start emulators
  //assert(rxMap.size()==cfgMap.size());
  for (const auto& chn : cfgMap) {
    int chn_tx, chn_tx2;
    std::tie(chn_tx, chn_tx2) = chn.first;

    auto tx = EmuTxCore<StarChips>::getCom(chn_tx);

    EmuCom* tx2 = nullptr;
    if (chn_tx2 >= 0) tx2 = EmuTxCore<StarChips>::getCom(chn_tx2);

    std::vector<ClipBoard<RawData>*>& rx = rxMap[chn.first];

    emus.emplace_back(new StarEmu(rx, tx, tx2, emuCfgFile, chn.second, hprperiod, abc_version, hcc_version));
    emuThreads.push_back(std::thread(&StarEmu::executeLoop, emus.back().get()));
  }
}
