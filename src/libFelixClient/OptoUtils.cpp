#include "OptoUtils.h"
#include "Utils.h"
#include "logging.h"


uint32_t OptoUtils::getDeviceAddress(std::string device_type, int device_number, uint32_t lpgbt_primary_addr = 116){
  uint32_t addr = 0;
  if (device_type == "GBCR" || "gbcr"){
    addr = 32 + device_number;
  }
  elif (device_type == "LPGBT" || "LpGBT" || "lpgbt"){
    addr = lpgbt_primary_addr + 1 + device_number;
  }
  else {
    std::cerr << "Invalid provided device type: " << device_type << ", accepted options are GBCR, gbcr, lpgbt, LpGBT, and LPGBT" << std::endl;
  }
  return addr;
}

std::vector<uint8_t> OptoUtils::prepareICDataFrame(const bool read, const uint16_t reg_addr, const uint8_t i2c_addr, const unsigned int device_version, const std::vector<uint8_t>& data){
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

  std::vector<uint8_t> frame = {};
  
  size_t header_size = 6;
  if(deviceVersion == 0){
    header_size = 7;
  }
  constexpr size_t footer_size = 1;

  if (device_version == 1 && data.size() > 511 ){
    std::cerr << "Size of data packet is too large to send for this version of LpGBT (v.1), maximum size is 511, current size is " << data.size()<< std::endl;
    return frame;
  }

  // The size of the frame we send depends on whether this is a read or write command
  if(!read)
    frame.reserve(header_size + data.size() + footer_size);
  else
    frame.reserve(header_size + footer_size);

  if(device_version == 0)
    frame.push_back(0); // Reserved in LpGBT v0
  
  frame.push_back((i2c_addr << 1) + (read?0x1:0x0)); // GBTX I2C address and read/write bit
  frame.push_back(1); // Command (not used in GBTX v1 + 2)
  frame.push_back(data.size() & 0xFF); // Number of data bytes
  frame.push_back((data.size() >> 8) & 0xFF);
  
  frame.push_back(reg_addr & 0xFF); // Register (start) address
  frame.push_back(reg_addr >> 8 & 0xFF);

  if(!read){
    for(auto& val: data){
      frame.push_back(val);
    }
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