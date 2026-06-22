#ifndef STAR_PROCESSOR_SIMPLE_H
#define STAR_PROCESSOR_SIMPLE_H

#include "StarProcessor.h"

/**
 * Some simple processors with only one or two functions.
 */
namespace StarProcessors {

struct PacketType : public EmptyProc {
  std::optional<int> type{};

  void packet_type(int t) {
    type = t;
  }
};

struct ReadHccRegister : public EmptyProc {
  std::optional<std::tuple<uint8_t, uint32_t>> result{};

  void hcc_read(bool hpr_not_rr, uint8_t read_address, uint32_t read_value) {
    if(hpr_not_rr) return;

    result = std::make_tuple(read_address, read_value);
  }
};

struct ReadAbcRegister : public EmptyProc {
  std::optional<std::tuple<uint8_t, uint8_t, uint32_t, uint16_t>> result;

  void abc_read(bool hpr_not_rr, int ic, int address, int value, int status) {
    if(hpr_not_rr) {
      return;
    }

    result = std::make_tuple(ic, address, value, status);
  }
};

} // End namespace

#endif
