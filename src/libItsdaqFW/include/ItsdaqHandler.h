#ifndef ITSDAQ_HANDLER_H
#define ITSDAQ_HANDLER_H

#include "RawData.h"
#include "ClipBoard.h"

#include <map>
#include <memory>

class ItsdaqPrivate;

/// Common communication code to/from itsdaq FW
class ItsdaqHandler
{
  std::unique_ptr<ItsdaqPrivate> priv;

public:
  // Prevent copying and moving.
  ItsdaqHandler(ItsdaqHandler const&) = delete;             // Copy construct
  ItsdaqHandler(ItsdaqHandler&&) = delete;                  // Move construct
  ItsdaqHandler& operator=(ItsdaqHandler const&) = delete;  // Copy assign
  ItsdaqHandler& operator=(ItsdaqHandler &&) = delete;      // Move assign

  void SendOpcode(uint16_t opcode, uint16_t *data, uint16_t length);
  std::unique_ptr<RawData> GetData();

public:
  ItsdaqHandler(uint32_t remote = 0x16dea8c0, // 192.168.222.22
                uint16_t srcPort = 0, uint16_t dstPort = 60002);

  void reconfigure(uint32_t remote, uint16_t srcPort, uint16_t dstPort);

  ~ItsdaqHandler();

  const std::vector<uint16_t> &LatestStatus();
  const std::vector<uint16_t> &LatestSysStatus();
};

#endif
