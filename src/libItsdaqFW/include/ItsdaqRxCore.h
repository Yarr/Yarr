#ifndef ITSDAQ_RXCORE_H
#define ITSDAQ_RXCORE_H

#include "RxCore.h"

#include "RawData.h"
#include "ItsdaqHandler.h"

#include <cstdint>

#include "storage.hpp"

/**
 * Access to Itsdaq FW.
 **/
class ItsdaqRxCore : virtual public RxCore {
public:

  /**
   * Initialize.
   **/
  ItsdaqRxCore(ItsdaqHandler &);

  /**
   * Shutdown the ItsdaqHandler. Delete the channels from the ItsdaqHandler.
   **/
  ~ItsdaqRxCore() override;

  void setRxEnable(uint32_t val) override;
  void setRxEnable(std::vector<uint32_t> channels) override;
  void disableRx() override;
  void maskRxEnable(uint32_t val, uint32_t mask) override;

  void flushBuffer() override;
  std::vector<RawDataPtr> readData() override;

  /**
   * Get data rate (Mhz)
   **/
  uint32_t getDataRate() override;

  /**
   * Get estimated size of the next data
   **/
  uint32_t getCurCount() override;

  /**
   * Check if data might not yet be read
   **/
  bool isBridgeEmpty() override;

  /**
   * Write configuration to json.
   *
   * @param j json object to store configuration
   **/
  void writeConfig(json &j) const;

  /**
   * Read configuration from json.
   *
   * @param j json object to read configuration from
   **/
  void loadConfig(const json &j);

 protected:
  /// Do some initialisation after ItsdaqHandler is constructed
  void init();

private:
  /// Common communication handler
  ItsdaqHandler &m_h;

  /// Firmware stream configuration to use
  uint16_t m_streamConfig;
};

#endif

