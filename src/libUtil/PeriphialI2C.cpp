// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: I2C master interface
// # Comment: Base class for I2C devices
// ################################

#include "PeriphialI2C.h"
#include "I2CRegs.h"
#include <unistd.h>

PeriphialI2C::PeriphialI2C(SpecCom *arg_spec) {
    spec = arg_spec;
}

int PeriphialI2C::checkTip() {
    unsigned timeout_cnt = 0;
    while ((spec->readSingle(I2C_ADDR | I2C_STAT) & I2C_STAT_TIP) == I2C_STAT_TIP &&
            timeout_cnt < I2C_TIMEOUT)
        timeout_cnt++;
    if (timeout_cnt >= I2C_TIMEOUT)
        return -1;

    return 0;
}


void PeriphialI2C::init() {
    // Stop core
    spec->writeSingle(I2C_ADDR | I2C_CTRL, 0x0);
    // Program clock prescaler, assume 200MHz sys clock
    // value = (sys_clk/((5 x scl_clk) -1 )
    // We want around 1 MHz SCL clock
    uint32_t prescale = 50;
    spec->writeSingle(I2C_ADDR | I2C_PRE_LOW, prescale & 0xFF);
    spec->writeSingle(I2C_ADDR | I2C_PRE_HIGH, 0x0);
    // Start up core
    spec->writeSingle(I2C_ADDR | I2C_CTRL, I2C_CTRL_I2C_EN);
}

int PeriphialI2C::setAddr(uint32_t dev_addr, uint32_t reg_addr) {
    // Set Slave address and write bit
    spec->writeSingle(I2C_ADDR | I2C_TX, 0x0 | ((dev_addr << 1) & 0xfe));
    // Enable start and write in command reg
    spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_START);
    // Check TIP to make sure command is done
    if (this->checkTip()) {
        std::cerr << __PRETTY_FUNCTION__ << " : TIP timed out!" << std::endl;
        return -1;
    }

    // Set Slave reg addr
    spec->writeSingle(I2C_ADDR | I2C_TX, reg_addr & 0xFF);
    // Enable write
    spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR);
    // Check TIP to make sure command is done
    if (this->checkTip()) {
        std::cerr << __PRETTY_FUNCTION__ << " : TIP timed out!" << std::endl;
        return -1;
    }

    last_dev_addr = dev_addr;
    last_reg_addr = reg_addr;

    return 0;
}

int PeriphialI2C::writeData(uint32_t value) {
    // Set slave reg data
    spec->writeSingle(I2C_ADDR | I2C_TX, value & 0xFF);
    // Enable write
    spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_STOP);
    // Check TIP to make sure command is done
    if (checkTip()) {
        std::cerr << __PRETTY_FUNCTION__ << " : TIP timed out!" << std::endl;
        return -1;
    }

    return 0;
}

int PeriphialI2C::setupWrite(uint32_t dev_addr) {
    // Set Slave address and write bit
    spec->writeSingle(I2C_ADDR | I2C_TX, 0x0 | ((dev_addr << 1) & 0xfe));
    // Enable start and write in command reg
    spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_START);
    // Check TIP to make sure command is done
    if (this->checkTip()) {
        std::cerr << __PRETTY_FUNCTION__ << " : TIP timed out!" << std::endl;
        return -1;
    }

    return 0;
}


int PeriphialI2C::setupRead(uint32_t dev_addr) {
    // Set Slave address and read bit
    spec->writeSingle(I2C_ADDR | I2C_TX, 0x1 | ((dev_addr << 1) & 0xfe));
    // Enable start and write in command reg
    spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_START);
    // Check TIP to make sure command is done
    if (checkTip()) {
        std::cerr << __PRETTY_FUNCTION__ << " : TIP timed out!" << std::endl;
        return -1;
    }
    return 0;
}

int PeriphialI2C::readData(uint32_t *value) {
    // Issue read + ACK (I2C_CMD_ACK=0)
    spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_RD);
    // Check TIP to make sure command is done
    if (checkTip()) {
        std::cerr << __PRETTY_FUNCTION__ << " : TIP timed out!" << std::endl;
        return -1;
    }
    
    // Value should be present in TX/RX reg
    *value = spec->readSingle(I2C_ADDR | I2C_RX);

    return 0;
}

int PeriphialI2C::sendNack() {
    // Send nack
    spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_RD | I2C_CMD_ACK);
    // Check TIP to make sure command is done
    if (checkTip()) {
        std::cerr << __PRETTY_FUNCTION__ << " : TIP timed out!" << std::endl;
        return -1;
    }
    return 0;
}

int PeriphialI2C::readReg(uint32_t dev_addr, uint32_t reg_addr, uint32_t *value) {
    if(this->setAddr(dev_addr, reg_addr)) {
        std::cerr << __PRETTY_FUNCTION__ << " : Setting address failed!" << std::endl;
        return -1;
    }
    
    if(this->setupRead(dev_addr)) {
        std::cerr << __PRETTY_FUNCTION__ << " : Setup read failed!" << std::endl;
        return -1;
    }
    
    if(this->readData(value)) {
        std::cerr << __PRETTY_FUNCTION__ << " : Reading data failed!" << std::endl;
        return -1;
    }

    if(this->sendNack()) {
        std::cerr << __PRETTY_FUNCTION__ << " : Sending NACK failed!" << std::endl;
        return -1;
    }
    
    return 0;
}

int PeriphialI2C::writeReg(uint32_t dev_addr, uint32_t reg_addr, uint32_t value) {
    if(setAddr(dev_addr, reg_addr)) {
        std::cerr << __PRETTY_FUNCTION__ << " : Setting address failed!" << std::endl;
        return -1;
    }

   if(writeData(value)) {
       std::cerr << __PRETTY_FUNCTION__ << " : Writing data failed!" << std::endl;
       return -1;
   }

   return 0;
}


