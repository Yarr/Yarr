#include "StarCLIUtils.h"
#include "AllHwControllers.h"
#include "HwController.h"
#include "LCBUtils.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "StarCmd.h"
#include "logging.h"
#include "storage.hpp"

#include <chrono>
#include <memory>
#include <sstream>
#include <string>

namespace {

auto logger = logging::make_log("StarCLIUtils");

}

namespace StarCLIUtils {

void setLoggingDefaults(const std::string &programName) {
  json j;
  j["pattern"] = logging::defaultLogPattern;
  j["log_config"][0]["name"] = programName;
  j["log_config"][0]["level"] = "info";
  j["log_config"][1]["name"] = "StarCLIUtils";
  j["log_config"][1]["level"] = "info";
  logging::setupLoggers(j);
}

std::unique_ptr<HwController>
createHwController(const std::string &configPath) {
  logger->info("Using controller from {}", configPath);
  json controllerConfig = ScanHelper::openJsonFile(configPath);
  std::unique_ptr<HwController> hwCtrl =
      StdDict::getHwController(controllerConfig["ctrlCfg"]["type"]);
  if (!hwCtrl) {
    logger->error("Failed to select valid HwController\n");
    exit(1);
  }
  hwCtrl->loadConfig(controllerConfig["ctrlCfg"]["cfg"]);
  return hwCtrl;
}

void prepareTx(HwController &hwCtrl, uint32_t txChannel) {
  logger->info("Enabling Tx channel {}", txChannel);
  hwCtrl.toggleTrigAbort();
  hwCtrl.setCmdEnable(txChannel);
}

void prepareRx(HwController &hwCtrl, uint32_t rxChannel) {
  logger->info("Enabling Rx channel {}", rxChannel);
  hwCtrl.disableRx();
  hwCtrl.initRxChannels({rxChannel});
  hwCtrl.setRxEnable(rxChannel);
}

void sendL0a(HwController &hwCtrl, unsigned mask, unsigned tag, bool bcr) {
  logger->info("Building L0a with mask 0x{:x}, tag 0x{:x}, bcr {}", mask, tag,
               bcr);
  auto frame = LCB::l0a_mask(mask, tag, bcr);
  logger->info("Sending frame 0x{:04x}", frame);
  hwCtrl.writeFifo((LCB::IDLE << 16) + frame);
  hwCtrl.releaseFifo();
}

void sendFastCommand(HwController &hwCtrl, LCB::FastCmdType type,
                     uint8_t delay) {
  logger->info("Building fast command of type {}, delay {}",
               static_cast<int>(type), delay);
  auto frame = LCB::fast_command(type, delay);
  logger->info("Sending frame 0x{:04x}", frame);
  hwCtrl.writeFifo((LCB::IDLE << 16) + frame);
  hwCtrl.releaseFifo();
}

void sendRegisterCommand(HwController &hwCtrl, int hccId, int abcId,
                         int address, bool isRead, uint32_t value, bool isHcc) {
  logger->info("Building {} command for {} register at address "
               "{} of chip with HCC ID: {}, ABC ID: {} value 0x{:08x}",
               isRead ? "read" : "write", isHcc ? "HCC" : "ABC", address, hccId,
               abcId, value);

  StarCmd starCmd;
  auto frames =
      starCmd.command_sequence(hccId, abcId, address, isRead, value, isHcc);

  std::stringstream logFrames;
  for (size_t i = 0; i < (frames.size() / 2) + 1; i++) {
    uint32_t word = (frames[i * 2] << 16);
    if (i * 2 + 1 < frames.size()) {
      word += frames[i * 2 + 1];
    } else {
      word += LCB::IDLE;
    }

    logFrames << "0x" << std::hex << word << std::dec << " ";
    hwCtrl.writeFifo(word);
  }
  logger->info("Sending frames {}", logFrames.str());
  hwCtrl.releaseFifo();
}

int packetFromRawData(StarChipPacket &packet, RawData &data) {
  packet.clear();

  std::stringstream ss;
  ss << std::hex << std::setfill('0');

  packet.add_word(0x13C); // add SOP
  for (unsigned iw = 0; iw < data.getSize(); iw++) {
    for (int i = 0; i < 4; i++) {
      auto byte = (data[iw] >> i * 8) & 0xFF;
      packet.add_word(byte);

      if (logger->should_log(spdlog::level::trace)) {
        ss << ' ' << std::setw(2) << static_cast<unsigned>(byte);
      }
    }
  }
  packet.add_word(0x1DC); // add EOP

  logger->trace("Raw data: {}", ss.str());

  return packet.parse();
}

bool isFromChannel(RawData &data, uint32_t chn) { return data.getAdr() == chn; }

bool isPacketType(RawData &data, PacketType packet_type) {
  StarChipPacket packet;
  if (packetFromRawData(packet, data)) {
    // failed to parse the packet
    return false;
  } else {
    // successfully parsed the packet

    if (packet.getType() == TYP_ABC_TRANSP) {
      logger->debug("Received a Packet Transparent packet");
      // Check the type of the forwarded ABCStar packet
      // First byte is TYP_ABC_TRANSP and channel number
      // Type of the ABCStar packet is the top 4 bits of the second byte
      int raw_type_abc = (data[0] & 0xf000) >> 12;
      if (packet_type_headers.find(raw_type_abc) == packet_type_headers.end()) {
        logger->error("Packet type was parsed as {}, which is an invalid type.",
                      raw_type_abc);
        return false;
      }
      // compare the forwarded ABC packet type to the expected type
      return packet_type_headers[raw_type_abc] == packet_type;

    } else {
      // compare the packet type to the expected type
      return packet.getType() == packet_type;
    }
  }
}

std::vector<RawDataPtr> readData(HwController &hwCtrl,
                    std::function<bool(RawData &)> filter_cb,
                    uint32_t timeout) {
  logger->info("Reading data");
  bool nodata = true;
  bool done = false;

  std::vector<RawDataPtr> output;
  std::vector<RawDataPtr> dataVec;

  auto start_reading = std::chrono::steady_clock::now();
  while (not done) {
    dataVec = hwCtrl.readData();

    if (not dataVec.empty()) {
      nodata = false;
      for (const auto &d : dataVec) {
        logger->trace("Use data: {}", (void *)d->getBuf());

        // check if it is the type of data we want
        bool good = filter_cb(*d);
        if (good) {
          output.push_back(d);
          break;
        }
      }
    } else { // no data
      // wait a bit
      static const auto SLEEP_TIME = std::chrono::milliseconds(1);
      std::this_thread::sleep_for(SLEEP_TIME);
    }

    auto run_time = std::chrono::steady_clock::now() - start_reading;
    done = (run_time > std::chrono::milliseconds(timeout));
  }

  if (nodata) {
    logger->error("No data was received");
    exit(1);
  } else if (output.empty()) {
    logger->error("Data was received, but none was of the expected type");
    exit(1);
  }

  return output;
}

} // namespace StarCLIUtils
