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
    geo.nRow = 0;
    geo.nCol = 0;
}

StarChips::StarChips(HwController *arg_core)
  : StarCmd(), FrontEnd()
{
    m_txcore  = arg_core;
    m_rxcore = arg_core;
    txChannel = 99;
    rxChannel = 99;
    active = true;
    geo.nRow = 0;
    geo.nCol = 0;
}

StarChips::StarChips(HwController *arg_core, unsigned arg_channel)
  : StarCmd(), FrontEnd()
{
    m_txcore  = arg_core;
    m_rxcore = arg_core;
    txChannel = arg_channel;
    rxChannel = arg_channel;

    active = true;
    geo.nRow = 0;
    geo.nCol = 0;
}

StarChips::StarChips(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel)
  : StarCmd(), FrontEnd()
{
    m_txcore  = arg_core;
    m_rxcore = arg_core;
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;

    active = true;
    geo.nRow = 0;
    geo.nCol = 0;
}

void StarChips::init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    m_txcore  = arg_core;
    m_rxcore = arg_core;
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    active = true;
}

void StarChips::reset(){
	std::cout << "Global reseting all HCC and ABC on the same control segment " << std::endl;

	// global resets registers

//	write(ABC_SOFT_RESET);
//	write(ABC_FASTCLUSTERFINDER_RESET);
//    write(ABC_BC_RESET);
//    write(ABC_L0ID_RESET);
//    write(ABC_SEU_RESET);
//
//    write(HCC_SOFT_RESET);
//	write(HCC_LOGIC_RESET);
//
//	write(SYS_RESET);

}

void StarChips::configure() {
	m_txcore->writeFifo(0x0100);
	m_txcore->writeFifo(0xFFFF);
	m_txcore->writeFifo(0x0100);
	m_txcore->writeFifo(0xFFFF);
//	this->writeRegisters();
	m_txcore->releaseFifo();

}

void StarChips::sendCmd(std::array<uint16_t, 9> cmd){
//    std::cout << __PRETTY_FUNCTION__ << "  txChannel: " << getTxChannel() << " cmd:  " <<  cmd << std::endl;

	m_txcore->writeFifo((cmd[0] << 16) + cmd[1]);
	m_txcore->writeFifo((cmd[2] << 16) + cmd[3]);
	m_txcore->writeFifo((cmd[4] << 16) + cmd[5]);
	m_txcore->writeFifo((cmd[6] << 16) + cmd[7]);
	m_txcore->writeFifo((cmd[8] << 16) + 0);
	m_txcore->releaseFifo();

}


bool StarChips::writeRegisters(){
	//Write all register to their setting, both for HCC & all ABCs
	//std::cout << "!!!! m_nABC is " << m_nABC << std::endl;
	for( int iChip = 0; iChip < m_nABC+1; ++iChip){
		int this_chipID = m_chipIDs[iChip];
		//std::cout << "Starting on chip " << this_chipID << " with length " << registerMap[this_chipID].size() << std::endl;
		std::map<unsigned, Register*>::iterator map_iter;
		for(map_iter=registerMap[this_chipID].begin(); map_iter!= registerMap[this_chipID].end(); ++map_iter){
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			setAndWriteRegister(map_iter->first, -1, this_chipID);
		}
		//std::cout << "Done with " << iChip << std::endl;
	}

	return true;
}


void StarChips::writeNamedRegister(std::string n, uint16_t val) {
  // look up in register map.
}


void StarChips::readRegisters(){


	//Read all known registers, both for HCC & all ABCs
	if(m_debug)  std::cout << "Looping over all chips in readRegisters, where m_nABC is " << m_nABC << " and m_chipIDs size is " <<  m_chipIDs.size() << std::endl;
	for( int iChip = 0; iChip < m_nABC+1; ++iChip){

		int this_chipID = m_chipIDs[iChip];
		std::map<unsigned, Register*>::iterator map_iter;
		for(map_iter=registerMap[this_chipID].begin(); map_iter!= registerMap[this_chipID].end(); ++map_iter){
			if( m_debug ) {
				std::cout <<"Hcc id: " << m_hccID << std::endl;
				std::cout << "Calling readRegister for chipID " << this_chipID << " register " << map_iter->first << std::endl;

			}
			readRegister(map_iter->first, this_chipID);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if(m_debug)
				std::cout << "Calling read()" << std::endl;
//			read(map_iter->first, rxcore);
		}//for each register address
	}//for each chipID

}


//NOTE:: need chipID -- but involve change to FrontEnd.h
uint32_t StarChips::getRegister(uint32_t addr, int32_t chipID){
	return registerMap[chipID][addr]->getValue();
}

void StarChips::setRegister(uint32_t addr, uint32_t val, int32_t chipID){
	registerMap[chipID][addr]->setValue(val);
}





void StarChips::toFileJson(json &j){
    StarCfg::toFileJson(j);
}

void StarChips::fromFileJson(json &j){
    StarCfg::fromFileJson(j);
}

