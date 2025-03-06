#include "OptoUtils.h"
#include "Utils.h"
#include "logging.h"

 bool OptoUtils::communicateOptoDevice(lpgbt_addr, i2c_addr, regname, regval, read){
  regAddr = getRegaddr();
  data = prepareFrame(addresses);
  readback(communicate)
 }

bool OptoUtils::readSecondaryDeviceI2C(uint_32t, reg_addr, uint8_t& reg_data){
  
}

bool OptoUtils::readWriteReg(uint_32t reg_addr, bool write, int lpgbt_addr, int i2c_addr, uint8_t reg_data = 0){
  std::string i2C_addr_str = std::static_cast<string>(i2c_addr);

  // if we're communicating directly to the primary LpGBT, we only need to send one simple register read
  if (lpgbt_addr == m_i2c_addr){
    communicateOptoDevice(red_addr, reg_data);
  }
  // communicating with secondary LpGBTs via I2C channel through the primary LpGBT
  else {
    uint8_t NBYTE = 2;
    communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "DATA0"], (m_scl_drive << 7) | (NBYTE << 2) | m_freq)
    communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "CMD"], m_i2c_write_cr)

    // Send address of register to read
    communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "DATA0"], divmod(reg_addr,0x100)[1])    //Lower half of register address
    communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "DATA1"], divmod(reg_addr,0x100)[0])    //Upper half of register address
    if (write){
      communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "DATA2"], reg_data)    //Upper half of register address
    }

    communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "CMD"], m_i2c_w_multi_4byte0)
    communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "ADDRESS"], lpgbt_addr)
    communicateOptoDevice(LPGBT_REGMAP["I2CM" + i2c_addr_str + "CMD"], m_i2c_write_multi)    //Initiate send of register address and register value

    // Read back answer
    NBYTE = 1
    self.comm_wrapper(LPGBT_REGMAP["I2CM" + i2c_addr_str + "DATA0"], (m_scl_drive << 7) | (NBYTE << 2) | m_freq)
    self.comm_wrapper(LPGBT_REGMAP["I2CM" + i2c_addr_str + "CMD"], m_i2c_write_cr)

    self.comm_wrapper(LPGBT_REGMAP["I2CM" + i2c_addr_str + "ADDRESS"], lpgbt_addr)
    self.comm_wrapper(LPGBT_REGMAP["I2CM" + i2c_addr_str + "CMD"], m_i2c_read_multi)

    status_value = self.comm_wrapper(LPGBT_REGMAP["I2CM" + i2c_addr_str + "STATUS"], None)
    self.status_info(status_value)     # Read status register

    # Read slave answer from LpGBT master register
    read_reg, read = self.comm_wrapper(LPGBT_REGMAP["I2CM" + i2c_addr_str + "READ15"], None)

  if reg_field is not None:

      reg_field_class = getattr(getattr(eval("self." + self.device_type + "_reg_map"), reg), reg_field)
      read = read >> getattr(reg_field_class, "offset") & (2**getattr(reg_field_class, "length")-1)

      logger.debug("read - reg_field is not None - offset: %s, length: %s", getattr(reg_field_class, "offset"), getattr(reg_field_class, "length"))

      read_reg = reg + "_" + reg_field
  else:
      read_reg = reg      # would otherwise return "I2CM" + str(I2C_master) + "READ15"

  logger.debug("Read from %s - %s: %s (%s, %s)", self.device, read_reg, read, hex(read), bin(read))

  return read
}
*/

bool OptoUtils::writeLpGBTRegister(uint32_t reg_addr, uint8_t& reg_data){


}

bool OptoUtils::readGBCRRegister(uint32_t reg_addr, uint8_t& reg_data){


}

bool OptoUtils::writeGBCRRegister(uint32_t reg_addr, uint8_t& reg_data){


}

std::vector<uint8_t> FelixController::prepareICDataFrame(const bool read, const uint16_t reg_addr, const uint8_t i2c_addr, const unsigned int device_version, const std::vector<uint8_t>& data){
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