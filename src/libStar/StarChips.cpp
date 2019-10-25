// #################################
// # Project: Yarr
// # Description: StarChips Library
// # Comment: StarChip FrontEnd class
// ################################

#include "StarChips.h"

#include <bitset>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

#include "AllChips.h"

bool star_chips_registered =
StdDict::registerFrontEnd
  ("Star", []() { return std::unique_ptr<FrontEnd>(new StarChips); });

StarChips::StarChips()
: StarCmd(), FrontEnd()
{
	txChannel = 99;
	rxChannel = 99;
	active = false;
	geo.nRow = 2;
	geo.nCol = 128;


	//Create dummy configuration as placeholder for globalFe in preScan routines
	m_hccID = 0xf;
	m_nABC = 1;
	addABCchipID(0xf);
	this->initRegisterMaps(); //Initialize default register maps for 1 dummy ABC

}

StarChips::StarChips(HwController *arg_core)
: StarCmd(), FrontEnd()
{
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = 99;
	rxChannel = 99;
	active = true;
	geo.nRow = 2;
	geo.nCol = 128;

}

StarChips::StarChips(HwController *arg_core, unsigned arg_channel)
: StarCmd(), FrontEnd()
{
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = arg_channel;
	rxChannel = arg_channel;

	active = true;
	geo.nRow = 2;
	geo.nCol = 128;
}

StarChips::StarChips(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel)
: StarCmd(), FrontEnd()
{
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;

	active = true;
	geo.nRow = 2;
	geo.nCol = 128;
}

void StarChips::init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
	active = true;

	active = true;
	geo.nRow = 2;
	geo.nCol = 128;
}


void StarChips::setHccId(unsigned hccID) {
  //First step will consist in setting the HCC ID (serial number might be different depending on fuse !)
  //Load the eFuse serial number (and stop HPR)
  sendCmd(write_hcc_register(16, 0x5, 0xf));
  //Let's reset the HCC ID with a broadcast write of the HCCID+SN on reg 17
  uint32_t newReg17val = (hccID<<28) | m_sn;
  sendCmd(write_hcc_register(17, newReg17val, 0xf));
  std::cout << "Set HCC ID to " << hccID << " (sent on reg17 0x" << std::hex << std::setfill('0') << std::setw(8) << newReg17val << ")" << std::endl;

}



void StarChips::reset(){
	std::cout << "Global reseting all HCC and ABC on the same LCB control segment " << std::endl;

	uint8_t delay = 0; //2 bits BC delay

	//sendCmd(LCB::fast_command(LCB::LOGIC_RESET, delay) );
	std::cout << "Sending fast command #" << LCB::ABC_REG_RESET << " ABC_REG_RESET" << std::endl;
	sendCmd(LCB::fast_command(LCB::ABC_REG_RESET, delay) );

	std::cout << "Sending fast command #" << LCB::ABC_SLOW_COMMAND_RESET << " ABC_SLOW_COMMAND_RESET" << std::endl;
	sendCmd(LCB::fast_command(LCB::ABC_SLOW_COMMAND_RESET, delay) );

	std::cout << "Sending fast command #" << LCB::ABC_SEU_RESET << " ABC_SEU_RESET" << std::endl;
	sendCmd(LCB::fast_command(LCB::ABC_SEU_RESET, delay) );

	std::cout << "Sending fast command #" << LCB::ABC_HIT_COUNT_RESET << " ABC_HIT_COUNT_RESET" << std::endl;
	sendCmd(LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, delay) );

	std::cout << "Sending fast command #" << LCB::ABC_HITCOUNT_START << " ABC_HITCOUNT_START" << std::endl;
	sendCmd(LCB::fast_command(LCB::ABC_HITCOUNT_START, delay) );

	std::cout << "Sending fast command #" << LCB::ABC_START_PRLP << " ABC_START_PRLP" << std::endl;
	sendCmd(LCB::fast_command(LCB::ABC_START_PRLP, delay) );

	std::cout << "Sending lonely_BCR" << std::endl;
	sendCmd(LCB::lonely_bcr());
}

void StarChips::configure() {

	//Set the HCC ID
	if (m_sn) this->setHccId(m_hccID);

	std::cout << "Sending fast command #" << LCB::HCC_REG_RESET << " HCC_REG_RESET" << std::endl;
	this->sendCmd(LCB::fast_command(LCB::HCC_REG_RESET, 0) );

	std::cout << "Sending registers configuration..." << std::endl;

	this->writeRegisters();

}

void StarChips::sendCmd(uint16_t cmd){
	//	std::cout << std::hex <<cmd << std::dec<< "_"<<std::endl;

	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((cmd << 16) + 0);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->releaseFifo();

}

void StarChips::sendCmd(std::array<uint16_t, 9> cmd){
	//    std::cout << __PRETTY_FUNCTION__ << "  txChannel: " << getTxChannel() << " cmd:  " <<  cmd << std::endl;
	//	for( auto a : cmd ) {
	//		std::cout << std::hex <<a << std::dec<< "_";
	//	}
	//	std::cout <<  std::endl;

	//	std::cout << std::hex <<((cmd[0] << 16) + cmd[1])<< std::dec<< "_";
	//	std::cout << std::hex <<((cmd[2] << 16) + cmd[3]) << std::dec<< "_";
	//	std::cout << std::hex <<((cmd[4] << 16) + cmd[5])<< std::dec<< "_";
	//	std::cout << std::hex <<((cmd[6] << 16) + cmd[7])<< std::dec<< "_";
	//	std::cout << std::hex <<((cmd[8] << 16) + 0)<< std::dec<< "_";
	//	std::cout <<  std::endl;
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((cmd[0] << 16) + cmd[1]);
	m_txcore->writeFifo((cmd[2] << 16) + cmd[3]);
	m_txcore->writeFifo((cmd[4] << 16) + cmd[5]);
	m_txcore->writeFifo((cmd[6] << 16) + cmd[7]);
	m_txcore->writeFifo((cmd[8] << 16) + 0);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->releaseFifo();

}


bool StarChips::writeRegisters(){
	//Write all register to their setting, both for HCC & all ABCs
	std::cout << "!!!! m_nABC is " << m_nABC << std::endl;
	for( int iChip = 0; iChip < m_nABC+1; ++iChip){
	        int this_chipID = (iChip) ? getABCchipID(iChip) : getHCCchipID();
		if (iChip==1) this->reset();
		std::cout << "Starting on chip " << this_chipID << " with length " << registerMap[iChip].size() << " @ " << &registerMap << std::endl;
		std::map<unsigned, Register*>::iterator map_iter;
		for(map_iter=registerMap[iChip].begin(); map_iter!= registerMap[iChip].end(); ++map_iter){
			if( m_debug ) {
				std::cout << "Writing Register "<< map_iter->first << " for chipID " << this_chipID << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (iChip==0)
				setAndWriteHCCRegister(map_iter->first, -1);
			else
				setAndWriteABCRegister(map_iter->first, -1, iChip);
		}
		std::cout << "Done with " << iChip << std::endl;
	}

	return true;
}

//Will write value for setting name for the HCC if name starts with "HCC_" otherwise will write the setting for all ABCs if name starts with "ABCs_"
void StarChips::writeNamedRegister(std::string name, uint16_t reg_value) {
  std::string strPrefix = name.substr (0,4);
  //if we deal with a setting for the HCC, look up in register map.
  if (strPrefix=="HCC_")
    setAndWriteHCCSubRegister(name.substr(4), reg_value);
  else   if (strPrefix=="ABCs") {
    name = name.substr(5);
    if (m_debug) std::cout << "Writing " << reg_value << " on setting '" << name << "' for all ABCStar chips." << std::endl;
    for( int iChip = 1; iChip < m_nABC+1; ++iChip)
      setAndWriteABCSubRegisterForChipIndex(name, reg_value, iChip);
  }
}


const void StarChips::readRegisters(){

	//Read all known registers, both for HCC & all ABCs
	if(m_debug)  std::cout << "Looping over all chips in readRegisters, where m_nABC is " << m_nABC << std::endl;
	for( int iChip = 0; iChip < m_nABC+1; ++iChip){

	        int this_chipID = (iChip) ? getABCchipID(iChip) : getHCCchipID();
		std::map<unsigned, Register*>::iterator map_iter;
		for(map_iter=registerMap[iChip].begin(); map_iter!= registerMap[iChip].end(); ++map_iter){
			if(iChip==0 && map_iter->first==16) continue;
			if( m_debug ) {
				std::cout <<"Hcc id: " << getHCCchipID() << std::endl;
				std::cout << "Calling readRegister for chipID " << this_chipID << " register " << map_iter->first << std::endl;

			}
			if (iChip==0)
				readHCCRegister(map_iter->first);
			else
				readABCRegister(map_iter->first, this_chipID);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if(m_debug)
				std::cout << "Calling read()" << std::endl;
			//			read(map_iter->first, rxcore);
		}//for each register address
	}//for each chipID

}


void StarChips::toFileJson(json &j){
    StarCfg::toFileJson(j);
}

void StarChips::fromFileJson(json &j){
    StarCfg::fromFileJson(j);
}

