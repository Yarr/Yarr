#ifndef STAR_PROCESSOR_PRINT_H
#define STAR_PROCESSOR_PRINT_H

#include "StarProcessor.h"

#include <iomanip>
#include <iostream>

/// Processor that prints Simple info about packet
struct PrintProc : public EmptyProc {
  std::ostream &os;
  PrintProc() : os(std::cout) {}
  PrintProc(std::ostream &os) : os(os) {}

  void parse_error(const char *msg) {
    os << "Parse error: " << msg << '\n';
  }
  void packet_type(int t) {
    static std::array<std::string, 8> packet_type_names = {
      "TYP_PR",
      "TYP_LP",
      "TYP_ABC_RR",
      "TYP_ABC_TRANSP",
      "TYP_HCC_RR",
      "TYP_ABC_FULL",
      "TYP_ABC_HPR",
      "TYP_HCC_HPR",
    };

    os << "Packet type is " << packet_type_names[t] << '\n';
  }
  void begin_error_block() {
    os << "We received an error block 0x77F4!\n";
    os << "Received packet errors for the following channels:\n";
  }

  void error_info(int c, uint16_t m) {
    os << "  ";
    switch(c) {
    case 0: os << "ABC"; break;
    case 1: os << "BCID"; break;
    case 2: os << "L0tag"; break;
    case 3: os << "Timeout"; break;
    }
    os << " Error:";
    for(int i=0; i<11; i++) {
      if(m & (1<< i)) {
        os << " " << i;
      }
    }
    os << '\n';
  }

  void data_header(bool pr_not_lp, uint8_t bcid, bool parity, uint8_t l0id, int flag) {
    os << "Packet info: BCID " << (int)bcid << " (" << parity << "), L0ID "
              << (int)l0id << "\n";
  }

  void data_no_cluster(int ic) {
    if(cluster_count == 0) {
      os << "Packet's abc clusters are:\n";
      cluster_count = -1;
    }
    os << "  -) Empty chip (" << ic << ")\n";
  }

  void data_cluster(unsigned int ic, uint8_t addr, int next_map) {
    if(cluster_count == 0) {
      os << "Packet's abc clusters are:\n";
    } else if(cluster_count == -1) {
      cluster_count = 0;
    }
    os << "  " << (cluster_count ++) << ") InputChannel: " << ic
              << ", Address: 0x" << std::hex << (int)addr << std::dec << ", Next Strip Pattern: " << std::bitset<3>(next_map) << ".\n";
  }

  void abc_read(bool hpr_not_rr, int ic, int address, int value, int status) {
    os << " ABC " << ic << " ABC status " << std::hex << status << std::dec << " Address: " << address << " value: " << std::hex << std::setw(8) << std::setfill('0') << value << std::setfill(' ') << std::dec << '\n';
  }
  void hcc_read(bool hpr_not_rr, int address, int value) {
    os << " Address: " << address << " value: " << std::hex << value << std::dec << '\n';
  }

  void transparent_word(int ic, uint64_t word) {
    os << "ABC transparent " << ic << ": " << std::bitset<64>(word) << '\n';
  }

private:
  int cluster_count{};
};

#endif
