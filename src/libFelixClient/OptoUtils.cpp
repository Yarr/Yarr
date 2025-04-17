#include "OptoUtils.h"
#include "Utils.h"
#include "logging.h"

/*
uint32_t OptoUtils::getDeviceAddress(std::string device_type, int rx, uint32_t lpgbt_primary_addr){
  int device_number = getDeviceNum(rx);
  uint32_t addr = 0;
  if (device_type == "GBCR" || "gbcr"){
    addr = 32 + device_number;
  }
  else if (device_type == "LPGBT" || "LpGBT" || "lpgbt"){
    addr = lpgbt_primary_addr + 1 + device_number;
  }
  else {
    std::cerr << "Invalid provided device type: " << device_type << ", accepted options are GBCR, gbcr, lpgbt, LpGBT, and LPGBT" << std::endl;
  }
  return addr;
}
*/
/*
    getLink: Returns the FELIX link corresponding to a particular chip
        @param: rx (int): rx value for chip in connectivity file
        @return: link (int): FELIX link number

    Each lpGBT on the optoboard corresponds to one FELIX link
    Each lpGBT has six input lines from the front-end (egroups)
    The front-end rx values that map to these egroups count in increments of 4 and increase by 64 each lpGBT
    ex: lpGBT 1 (primary) has rx = {0,4,8,12,16,20}, link 00
        lpGBT 2 (secondary) has rx = {64,68,72,76,80,84}, link 01

    Calculate link number by integer dividing the rx by 64
    Note: This code currently only supports systems with one optoboard

    ex: rx = 76 should be link 01
        int(76 / 64) = 1
*/
int OptoUtils::getFELIXLinkForLpGBT(int rx){
  int link = rx/64;
  return link;
}

int OptoUtils::getDeviceNum(int rx){
    // 4 LpGBT per optoboard, so links 0, 1, 2, 3 should be LpGBTs 0,1,2,3, links 4,5,6,7 should be LpGBTs 0,1,2,3, etc.
    int link = getFELIXLinkForLpGBT(rx);
    int num = link/4;
    return num;
}

bool OptoUtils::isPrimaryLpGBT(int rx){
  bool is_primary = false;
  if (getDeviceNum(rx)==0){
    is_primary = true;
  }
  return is_primary;
}


std::vector<uint8_t> OptoUtils::prepareICDataFrame(const bool write, const uint16_t reg_addr, const uint8_t data, const uint16_t data_size){
  /*
    Based on itk-ic-over-netio-next communication wrapper, 
    source: https://gitlab.cern.ch/itk-felix-sw/itk-ic-over-netio-next/-/blob/master/src/itk-ic-over-netio-next.cc?ref_type=heads

    Data frame is structured as follows:
    header | data | footer
     * header: contains 6 or 7 bits depending on LpGBT version (first bit is 0 for v.0 LpGBT)
               constructed of i2CAddress, bit to designate if we're doing a read or write command, number of data bytes
     * data: if we're writing, we send the data to write, if not we skip this
     * footer: register address, parity check
  */
  uint8_t device_version = 1;
  uint8_t i2c_addr = 116;
  bool read = !write;
  std::vector<uint8_t> frame = {};
  
  size_t header_size = 6;
  if(device_version == 0){
    header_size = 7;
  }
  constexpr size_t footer_size = 1;

  if (device_version == 1 && data_size > 511 ){
    std::cerr << "Size of data packet is too large to send for this version of LpGBT (v.1), maximum size is 511, current size is " << data_size << std::endl;
    return frame;
  }

  // The size of the frame we send depends on whether this is a read or write command
  if(write)
    frame.reserve(header_size + data_size + footer_size);
  else
    frame.reserve(header_size + footer_size);

  if(device_version == 0){
    frame.push_back(0); // Reserved in LpGBT v0
  }

  std::cout << "what the hell " << std::hex << ((i2c_addr << 1) + (read?0x1:0x0)) <<std::endl;
  
  frame.push_back((i2c_addr << 1) + (read?0x1:0x0)); // GBTX I2C address and read/write bit
  frame.push_back(1); // Command (not used in GBTX v1 + 2)
  frame.push_back(data_size & 0xFF); // Number of data bytes
  frame.push_back((data_size >> 8) & 0xFF);
  
  frame.push_back(reg_addr & 0xFF); // Register (start) address
  frame.push_back((reg_addr >> 8) & 0xFF);

  if(write){
    frame.insert(frame.end(), data, data + data_size);
  }

  // For GBTx, skip first 2 bytes in parity check (see above)
  std::size_t NUM_PARITY_BYTES_SKIP{0};
  if(device_version == 0){
    NUM_PARITY_BYTES_SKIP=2;
  }
  uint8_t parity = 0;
  for(size_t i = NUM_PARITY_BYTES_SKIP;i < frame.size(); ++i){
    parity ^= frame[i];
  }
  frame.push_back(parity);

  return frame;
}