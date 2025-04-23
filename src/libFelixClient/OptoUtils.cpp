#include "OptoUtils.h"
#include "Utils.h"
#include "logging.h"

const lpgbt_item_t* OptoUtils::getLpGBTRegisterByName(const char* regname, uint8_t version) {
  // Get the correct item list based on the version
  const lpgbt_item_t* reg_list = 0;
  if (version == 0){
    reg_list = LPGBTv0_ITEM;
  }
  else if(version == 1){
    reg_list = LPGBTv1_ITEM;
  }
  else {
    std::cerr << "Invalid version provided: " << std::hex << static_cast<int>(version) << " version, only accepted values are 0 and 1." << std::endl;
  }

  const lpgbt_item_t*item = &reg_list[0];
  // loop through every item in the list
  bool no_match_found = false;
  while(strlen(item->name) != 0) {
    if(strcmp(item->name, regname) == 0){
        no_match_found = false;
        break;
      }
    else {
      no_match_found = true;
    }
    ++item;
  }

  if (no_match_found){
    std::cerr << "No match was found for register name " << regname << " in the register list" << std::endl;
  }

  return item;
}

const lpgbt_item_t* OptoUtils::getLpGBTRegisterByAddr(uint16_t reg_addr, uint8_t version){
  // Get the correct item list based on the version
  const lpgbt_item_t* reg_list = 0;
  if (version == 0){
    reg_list = LPGBTv0_ITEM;
  }
  else if(version == 1){
    reg_list = LPGBTv1_ITEM;
  }
  else {
    std::cerr << "Invalid version provided: " << std::hex << static_cast<int>(version) << " version, only accepted values are 0 and 1." << std::endl;
  }

  const lpgbt_item_t*item = &reg_list[0];
  // loop through every item in the list
  bool no_match_found = false;
  while(strlen(item->name) != 0) {
    if(item->addr == reg_addr){
        no_match_found = false;
        break;
      }
    else {
      no_match_found = true;
    }
    ++item;
  }

  if (no_match_found){
    std::cerr << "No match was found for register with address " << reg_addr << " in the register list" << std::endl;
  }

  return item;
}

std::vector<uint8_t> OptoUtils::prepareICDataFrame(const bool write, const uint16_t reg_addr, const uint8_t data, uint8_t version, uint16_t dev_addr){
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
  bool read = !write;
  std::vector<uint8_t> frame = {};
  
  size_t header_size = 6;
  if(version == 0){
    header_size = 7;
  }
  constexpr size_t footer_size = 1;

  // The size of the frame we send depends on whether this is a read or write command
  if(write)
    frame.reserve(header_size + 1 + footer_size);
  else
    frame.reserve(header_size + footer_size);

  if(version == 0){
    frame.push_back(0); // Reserved in LpGBT v0
  }
  
  frame.push_back((dev_addr << 1) + (read?0x1:0x0)); // GBTX I2C address and read/write bit
  frame.push_back(1); // Command (not used in GBTX v1 + 2)
  frame.push_back(1 & 0xFF); // Number of data bytes
  frame.push_back((1 >> 8) & 0xFF);
  
  frame.push_back(reg_addr & 0xFF); // Register (start) address
  frame.push_back((reg_addr >> 8) & 0xFF);

  if(write){
    frame.push_back(data);
  }
  // For GBTx, skip first 2 bytes in parity check (see above)
  std::size_t NUM_PARITY_BYTES_SKIP{0};
  if(version == 0){
    NUM_PARITY_BYTES_SKIP=2;
  }
  uint8_t parity = 0;
  for(size_t i = NUM_PARITY_BYTES_SKIP;i < frame.size(); ++i){
    parity ^= frame[i];
  }
  frame.push_back(parity);

  return frame;
}