#ifndef FELIX_DIRECT_MODE_H
#define FELIX_DIRECT_MODE_H

#include "LoopActionBase.h"

/**
 * Loop action to switch channels into direct mode.
 */
class FelixDirectMode : public LoopActionBase {
 public:
  FelixDirectMode();

  void writeConfig(json &config) override;
  void loadConfig(const json &config) override;

  void init() override;
  void end() override;
  void execPart1() override;
  void execPart2() override;

private:
  /// Set config for direct mode
  void setMode(int mask, int length);

  // Set direct mode enable (on enabled links)
  void setLinksEnable(int enable);

  uint16_t m_capture_length;
  uint16_t m_start_source_mask;
};

#endif
