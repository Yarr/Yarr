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

typedef std::array<uint32_t, 8> MaskType;

//"Ring" of channels ordered in the "physical" order
//Can return 32 bits sets ordered such as for:
//- the masks (1st column in 1st row, 1st column in 2nd row, 2nd column in 1st row, 2nd column in 2nd row, 3rd column in 1st row, 3rd column in 2nd row, 4th column in 1st row, 4th column in 2nd row,..., 127th column in 1st row, 127th column in 2nd row)
//- or as in "CAL ENABLE" registers is like 1st row 1st col, 1st row 2nd col, 2nd row 1st col, 2nd row 2nd col,...
class ChannelRing {
 public:
  ChannelRing() {}
  ~ChannelRing() {}
  uint16_t pos=0;

  /// Fill in hit data order
  void fill(uint32_t newBits, short size) {
    for (int i=size-1; i>=0; i--) {
      bits[pos] = (newBits >> i) & 0x1;
      pos++;
    }
  }
  void printRing() const { std::cout << "Ring content: "; for (unsigned int i=0;i<256; i++) std::cout << bits[i]; std::cout << "." << std::endl;}

  /// Extract register settings for the mask register
  MaskType readMask(int offset) const {
    MaskType masks;
    masks.fill(0);

    for (unsigned int curpos=0; curpos<256; curpos=curpos+4) { //Loop over the "blocks" of mask bits (1st row Xth col, 1st row X+1th col, 2nd row Xth col, 2nd row X+1th col)
      unsigned int posBlock = curpos%32;
      masks[curpos/32] |= bits[(offset+curpos/2)%256]      << posBlock;
      masks[curpos/32] |= bits[(offset+curpos/2+128)%256]  << (posBlock+1);
      masks[curpos/32] |= bits[(offset+curpos/2+1)%256]    << (posBlock+2);
      masks[curpos/32] |= bits[(offset+curpos/2+128+1)%256]<< (posBlock+3);
    }
    for(int m_i=0; m_i<8; m_i++) {
      auto &m = masks[m_i];
      m = ~m;
    }
    return masks;
  }
  /// Extract register settings for the calmask register
  MaskType readCalEnable(int offset) const {
    MaskType masks;
    masks.fill(0);

    for (unsigned int curpos=0; curpos<256; curpos=curpos+4) { //Loop over the "blocks" of mask bits (1st row Xth col, 1st row X+1th col, 2nd row Xth col, 2nd row X+1th col)
      unsigned int posBlock = curpos%32;
      masks[curpos/32] |= bits[(offset+curpos/2)%256]      << posBlock;
      masks[curpos/32] |= bits[(offset+curpos/2+1)%256]    << (posBlock+1);
      masks[curpos/32] |= bits[(offset+curpos/2+128)%256]  << (posBlock+2);
      masks[curpos/32] |= bits[(offset+curpos/2+128+1)%256]<< (posBlock+3);
    }
    return masks;
  }

 private:
  bool bits[512]={0};
};



class StarMaskLoop : public LoopActionBase {
 public:
  StarMaskLoop();

  void writeConfig(json &config);
  void loadConfig(json &config);

 protected:
  void applyMask(StarChips* fe, MaskType masks, MaskType enables);
  void applyEncodedMask(StarChips* fe, unsigned int curStep);
  void printMask(MaskType chans, std::ostream &os) const;
  void initMasks();
  
 private:
  short m_nMaskedStripsPerGroup,m_nEnabledStripsPerGroup, m_EnabledMaskedShift;
  unsigned m_cur;

  // ie. don't send calibration masks
  bool m_onlyMask;

  ChannelRing m_maskedChannelsRing, m_enabledChannelsRing;
  
  void init() override;
  void end() override;
  void execPart1() override;
  void execPart2() override;
};

#endif
