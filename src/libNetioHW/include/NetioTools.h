#ifndef NETIOTOOLS_H
#define NETIOTOOLS_H

/********************************
 * NetioTools
 * Author: Carlos.Solans@cern.ch
 * Description: Data format tools
 * Date: June 2017
 *********************************/

#include <cstdint>

namespace NetioTools {

  uint64_t channel2elink(uint32_t channel);
  uint32_t elink2channel(uint64_t elink);
  int32_t msb(uint32_t num);
  uint32_t pad(uint32_t num, uint32_t n);
  uint32_t conv4(uint32_t num, int32_t pos);
  uint32_t conv4R(uint32_t num, uint32_t pos);
  uint32_t flip2(uint32_t num);
  uint32_t flip4(uint32_t num);
  uint32_t reverse(uint32_t num);

}

#endif
