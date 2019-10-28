#ifndef STARMASKLOOP_H
#define STARMASKLOOP_H

/* Class: StarMaskLoop 
Author: O. Arnaez <Olivier.Arnaez@cern.ch>
Date: October 2019

Applies some pattern of masked/enabled channels during scans.
Two modes are available: 
  1) reading masks/cal.enable register values from StarMaskCal_En.cpp if nEnabledStripsPerGroup is set to 0 in the configuration. In this case the values between indices min and max from this file are read every "step" entries.
  2) applying patterns corresponding to groups of nEnabledStripsPerGroup (nMaskedStripsPerGroup) strips in a row that are shifted N="max" times for the pattern applied to CAL ENABLE registers (resp. masked channels) ; in this case the two patterns applied to enabled/masked channels are shifted by EnabledMaskedShift channels.
*/

#include <iostream>

#include "StarChips.h"
#include "LoopActionBase.h"

enum MASK_STAGE_STRIP { //TODO-change this or delete it
  MASK_STRIP_FROMFILE=16
};

class ChannelRing {
 public:
  ChannelRing() {};
  ~ChannelRing() {};
  uint16_t pos=0;
  void fill(uint32_t newBits, short size) {
    for (int i=size-1; i>=0; i--) {
      bits[pos] = (newBits >> i) & 0x1;
      pos++;
    }
  };
  const uint32_t * read() {
    uint32_t * masks = new uint32_t[8]; for (unsigned short i=0;i<8;i++) masks[i]=0;
    for (unsigned int curpos=0; curpos<256; curpos++) {
      masks[curpos/32] |= bits[(pos+curpos)%256]<< (31-(curpos%32));
    }
    return masks;
  };
 private:
  bool bits[512]={0};
};



class StarMaskLoop : public LoopActionBase {
 public:
  StarMaskLoop();

  void writeConfig(json &config);
  void loadConfig(json &config);

 protected:
  void applyMask(StarChips* fe, const uint32_t masks[8], const uint32_t enables[8]);
  void applyEncodedMask(StarChips* fe, unsigned int curStep);
  void printMask(const uint32_t chans[8]);
  void initMasks();
  
 private:
  short m_nMaskedStripsPerGroup,m_nEnabledStripsPerGroup, m_EnabledMaskedShift;
  unsigned m_cur;

  ChannelRing m_maskedChannelsRing, m_enabledChannelsRing;
  
  void init();
  void end();
  void execPart1();
  void execPart2();
};

#endif
