#include <sstream>
#include <iterator>
#include <stdexcept>

#include "StarSeqGenerator.h"
#include "LCBUtils.h"
#include "LCBFwUtils.h"
#include "StarCmd.h"

#include "logging.h"
namespace {
  auto sglog = logging::make_log("StarSeqGenerator");

  StarCmd star; // TODO: make StarCmd member functions static?
}

void StarSeqGenerator::printCommandFormat(std::ostream &os, bool verbose) {
  // IDLE
  os << "Idle frames: \"idle <number of frames>\"\n";
  if (verbose) {
    os << " Examples:\n";
    os << "  idle 2\n";
  }

  // L0
  os << "L0: \"l0 <4-bit mask in binary> <7-bit tag> [BCR]\"\n";
  if (verbose) {
    os << " Examples:\n";
    os << "  l0 0011 4\n";
    os << "  l0 1000 +14 BCR\n";
  }

  // Fast commmand
  os << "Fast command: \"fast <cmd type> <delay>\"\n";
  if (verbose) {
    os << " Examples:\n";
    os << "  fast 3 2\n";
    os << "  fast 10 2\n";
  }

  // Register command
  os << "Register read command: \"reg <abc|hcc> read <address> [hccID] [abcID]\"\n";
  os << "Register write command: \"reg <abc|hcc> write <address> [value] [hccID] [abcID]\"\n";
  if (verbose) {
    os << " Example:\n";
    os << "  reg hcc read 17\n";
    os << "  reg hcc read 17 1\n";
    os << "  reg hcc write 40 0x000001ff\n";
    os << "  reg hcc write 40 0x0000007f 6\n";
    os << "  reg abc read 6\n";
    os << "  reg abc read 4 0xf 2\n";
    os << "  reg abc write 2 0x01001067\n";
    os << "  reg abc write 2 0x02001067 0x1 10";
  }
}

void StarSeqGenerator::addIdle(unsigned nframes) {
  for (unsigned i=0; i<nframes; i++) {
    if (m_fw) {
      m_sequence.push_back(LCB_FELIX::IDLE);
    } else {
      m_sequence.push_back((LCB::IDLE >> 8) & 0xff); // high 8 bits
      m_sequence.push_back(LCB::IDLE & 0xff); // low 8 bits
    }
  }
}

void StarSeqGenerator::addIdle(const std::vector<std::string>& cmd_tokens) {
  // idle <number of frames>
  //assert(cmd_tokens.at(0) == "idle");
  unsigned nframes = std::stoi(cmd_tokens.at(1), nullptr, 0); //auto-detect base
  addIdle(nframes);
}

void StarSeqGenerator::addL0(uint8_t mask, uint8_t tag, bool bcr) {
  if (m_fw) {
    auto l0a = LCB_FELIX::l0a_mask(mask, tag, bcr);
    m_sequence.insert(m_sequence.end(), l0a.begin(), l0a.end());
  } else {
    LCB::Frame l0a = LCB::l0a_mask(mask, tag, bcr);
    m_sequence.push_back((l0a >> 8) & 0xff); // high 8 bits
    m_sequence.push_back(l0a & 0xff); // low 8 bits
  }
}

void StarSeqGenerator::addL0(const std::vector<std::string>& cmd_tokens) {
  // l0 <4-bit mask in binary> <7-bit tag> [BCR]
  //assert(cmd_tokens.at(0) == "l0")
  uint8_t mask = std::stoi(cmd_tokens.at(1), nullptr, 2) & 0xf; // base 2
  uint8_t tag = std::stoi(cmd_tokens.at(2), nullptr, 0) & 0x7f; // auto-detect base
  bool bcr = false;
  if (cmd_tokens.size() >= 4) {
    bcr = cmd_tokens[3] == "bcr" or cmd_tokens[3] == "BCR";
  }
  addL0(mask, tag, bcr);
}

void StarSeqGenerator::addFastCmd(uint8_t type, uint8_t delay) {
  if (m_fw) {
    auto fcmd = LCB_FELIX::fast_command(type, delay);
    m_sequence.insert(m_sequence.end(), fcmd.begin(), fcmd.end());
  } else {
    LCB::Frame fcmd = LCB::fast_command(static_cast<LCB::FastCmdType>(type), delay);
    m_sequence.push_back((fcmd >> 8) & 0xff); // high 8 bits
    m_sequence.push_back(fcmd & 0xff); // low 8 bits
  }
}

void StarSeqGenerator::addFastCmd(const std::vector<std::string>& cmd_tokens) {
  // fast <cmd type> <delay>
  //assert(cmd_tokens.at(0) == "fast")
  addFastCmd(
    std::stoi(cmd_tokens.at(1), nullptr, 0), // auto-detect base
    std::stoi(cmd_tokens.at(2), nullptr, 0) // auto-detect base
  );
}

void StarSeqGenerator::addRegWrCmd(bool isABC, uint32_t address, uint32_t value, uint8_t hccID, uint8_t abcID) {
  if (m_fw) {
    if (isABC) {
      auto regcmd = LCB_FELIX::write_abc_register(address, value, hccID, abcID);
      m_sequence.insert(m_sequence.end(), regcmd.begin(), regcmd.end());
    } else {
      auto regcmd = LCB_FELIX::write_hcc_register(address, value, hccID);
      m_sequence.insert(m_sequence.end(), regcmd.begin(), regcmd.end());
    }
  } else {
    auto regcmd = isABC ? star.write_abc_register(address, value, hccID, abcID) : star.write_hcc_register(address, value, hccID);
    for (const uint16_t& frame : regcmd ) {
      m_sequence.push_back((frame >> 8) & 0xff);
      m_sequence.push_back(frame & 0xff);
    }
  }
}

void StarSeqGenerator::addRegRdCmd(bool isABC, uint32_t address, uint8_t hccID, uint8_t abcID) {
  if (m_fw) {
    if (isABC) {
      auto regcmd = LCB_FELIX::read_abc_register(address, hccID, abcID);
      m_sequence.insert(m_sequence.end(), regcmd.begin(), regcmd.end());
    } else {
      auto regcmd = LCB_FELIX::read_hcc_register(address, hccID);
      m_sequence.insert(m_sequence.end(), regcmd.begin(), regcmd.end());
    }
  } else {
    auto regcmd = isABC ? star.read_abc_register(address, hccID, abcID) : star.read_hcc_register(address, hccID);
    for (const uint16_t& frame : regcmd) {
      m_sequence.push_back((frame >> 8) & 0xff);
      m_sequence.push_back(frame & 0xff);
    }
  }
}

void StarSeqGenerator::addRegCmd(const std::vector<std::string>& cmd_tokens) {
  // reg <abc|hcc> read <address> [hccID] [abcID]
  // reg <abc|hcc> write <address> <value> [hccID] [abcID]
  //assert(cmd_tokens.at(0) == "reg")
  std::string chip = cmd_tokens.at(1);
  bool isABC = chip=="abc" or chip=="ABC";
  bool isHCC = chip=="hcc" or chip=="HCC";
  if (not isABC xor isHCC) {
    throw std::invalid_argument("Unknown chip type for register command: "+chip);
  }

  std::string access = cmd_tokens.at(2);
  bool isWrite = access=="write";
  if (access != "read" and access != "write") {
    throw std::invalid_argument("Unknown access type for register command: "+access);
  }

  unsigned addr = std::stoi(cmd_tokens.at(3), nullptr, 0); // auto-detect base

  if (isWrite) {
    unsigned value = std::stoi(cmd_tokens.at(4), nullptr, 0); // auto-detect base
    // HCC ID if specified otherwise broadcast by default
    unsigned hccID = 0xf;
    if (cmd_tokens.size() > 5) {
      hccID = std::stoi(cmd_tokens[5], nullptr, 0); // auto-detect base
    }
    // ABC ID if specified otherwise broadcast by default
    unsigned  abcID = 0xf;
    if (cmd_tokens.size() > 6) {
      abcID = std::stoi(cmd_tokens[6], nullptr, 0); // auto-detect base
    }
    addRegWrCmd(isABC, addr, value, hccID, abcID);
  } else {
    // HCC ID if specified otherwise broadcast by default
    unsigned hccID = 0xf;
    if (cmd_tokens.size() > 4) {
      hccID = std::stoi(cmd_tokens[4], nullptr, 0); // auto-detect base
    }
    // ABC ID if specified otherwise broadcast by default
    unsigned  abcID = 0xf;
    if (cmd_tokens.size() > 5) {
      abcID = std::stoi(cmd_tokens[5], nullptr, 0); // auto-detect base
    }
    addRegRdCmd(isABC, addr, hccID, abcID);
  }
}

void StarSeqGenerator::addCommand(const std::string& command) {

  // Split command string into tokens https://stackoverflow.com/a/237280
  std::istringstream cmd_ss(command);
  std::vector<std::string> tokens{
    std::istream_iterator<std::string>{cmd_ss}, std::istream_iterator<std::string>{}
  };

  std::string cmd_type = tokens.at(0);

  if (cmd_type == "idle") {
    addIdle(tokens);
  } else if (cmd_type == "l0") {
    addL0(tokens);
  } else if (cmd_type == "fast") {
    addFastCmd(tokens);
  } else if (cmd_type == "reg") {
    addRegCmd(tokens);
  } else {
    throw std::invalid_argument("Unrecognized command type: "+tokens[0]);
  }

}

bool StarSeqGenerator::parseCommandString(const std::string& command, bool verbose) {
  bool success {true};

  try {
    addCommand(command);
  } catch (const std::logic_error& ex) { // std::invalid_argument or std::out_of_range
    sglog->error("{}", ex.what());
    sglog->error("Failed to parse the command: {}", command);
    if (verbose) {
      std::stringstream helpmsg;
      printCommandFormat(helpmsg, true);
      sglog->info(helpmsg.str());
    }
    clear();
    success = false;
  }

  return success;
}

bool StarSeqGenerator::parseCommandSequence(const std::vector<std::string>& commands, bool verbose) {
  bool success {true};

  if (commands.empty() and verbose) {
    std::stringstream helpmsg;
    printCommandFormat(helpmsg, true);
    sglog->info(helpmsg.str());
    success = false;
  }

  for (const auto& cmd : commands) {
    try {
      addCommand(cmd);
    } catch (const std::logic_error& ex) { // std::invalid_argument or std::out_of_range
      sglog->error("{}", ex.what());
      sglog->error("Failed to parse the command: {}", cmd);
      if (verbose) {
        std::stringstream helpmsg;
        printCommandFormat(helpmsg, true);
        sglog->info(helpmsg.str());
      }
      clear();
      success = false;
      break;
    }
  }

  return success;
}