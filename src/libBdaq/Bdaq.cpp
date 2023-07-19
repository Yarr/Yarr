#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>


#include "Bdaq.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("Bdaq");
}

Bdaq::Bdaq() : 
	i2c(rbcp),
	si570(i2c),
	cmd(rbcp),
	fifo(tcp),
	bdaqControl(rbcp) {}

void Bdaq::initialize(bdaqConfig c) {
	// Initialize Remote Bus Control Protocol (RBCP)
	rbcp.connect(c.ipAddr, c.udpPort);

	// Get DAQ (board) version
 	dv = getDaqVersion();
	if (std::stod(VERSION) < std::stod(dv.fwVersion)) {
		std::string error = "Firmware version " + dv.fwVersion +
		" is less then version " + VERSION + "! Please update.";
		logger->critical(error);
		exit(-1);
	}
	if (dv.boardVersion != "BDAQ53")
	{
		std::string error = "Only BDAQ53 board is supported in \"bdaq\" "
			"controller mode.";
		logger->critical(error);
		exit(-1);
	}
	logger->info("\033[1;32mFound board " + dv.boardVersion + " with " +
		dv.connectorVersion + " running firmware version " +
		dv.fwVersion + "\033[0m");
	logger->info("Board has " + std::to_string(dv.numRxChannels) +
		" Aurora receiver channel(s)");

	// Initialize FPGA modules/drivers
	rx.reserve(7);
	for (uint i=0;i<7;++i) {
		rx.emplace_back(BdaqAuroraRx(rbcp));
		rx.at(i).setBase(c.rxAddr+0x100*i);
		rx.at(i).checkVersion();
	}

	i2c.setBase(c.i2cAddr);
	i2c.checkVersion();
	i2c.init();
	cmd.setBase(c.cmdAddr);
	cmd.checkVersion();
	cmd.init();
	tcp.connect(c.ipAddr, c.tcpPort);

	// BDAQ Control Register: Work in progress
	bdaqControl.setBase(c.controlAddr);
	bdaqControl.init(24, uint(pow(2.0, 24.0) - 1));
	bdaqControl.setData(0);

	//Check if Si570 is configured. If not, configure it.
 	//if (auroraRx.getSi570IsConfigured() == false) { // Commented due to BDAQ CONTROL WIP
	if (c.configSi570) {
		cmd.setOutputEn(false);
		for (uint i=0;i<7;++i) rx.at(i).reset();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		si570.init(0xBA, 160.0); //0xBA is the Si570 i2c slave address, 160 MHz.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		cmd.setOutputEn(true);
	} else {
		logger->info("Si570 (MGT ref. clock) configuration is disabled");
	}
	/*} else  // Commented due to BDAQ CONTROL WIP
		logger->info("Si570 oscillator is already configured");*/

	//Reset BdaqDriver (command TX)
	cmd.reset();

        if(c.feType == "RD53A"){
            //Setting BdaqDriver to RD53A
            setChipTypeRD53A();  // Move it to "main" code, when "rd53b_devel" is merged
            chipType = 0 ;
        }else if(c.feType == "RD53B"){
            //Setting BdaqDriver to RD53B
            setChipTypeITkPixV1();
            chipType = 1 ;
        }else{
            std::stringstream error;
            error << __PRETTY_FUNCTION__ << ": chip type is undefined, it should be either RD53A or Rd53B";
            logger->critical(error.str());
            exit(-1);
        }

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//Configure the Aurora link
	waitForPllLock();
	setupAurora();
}

daqVersion Bdaq::getDaqVersion() {
	std::vector<uint8_t> buf(2);
	daqVersion dv;
	//Firmware version
	rbcp.read(0x0, buf, 2);
        dv.fwVersion = std::to_string(buf.at(0)) + "." + std::to_string(buf.at(1));// for frimware = 1.8
	//Board version
	rbcp.read(0x2, buf, 2);
	dv.boardVersion = hwMap[buf.at(0)];
	//Number of RX channels
	rbcp.read(0x3, buf, 1);
	dv.numRxChannels = buf.at(0);
	//Board options
	rbcp.read(0x4, buf, 1);
//	dv.boardOptions = buf.at(0); // for frimware = 1.2
        dv.boardOptions = 0x01;      // for frimware = 1.8 (To be fixed "not accepted")
	//Connector Version
	rbcp.read(0x5, buf, 2);
       	dv.connectorVersion = hwConMap[buf.at(1)];
	return dv;
}

/*void Bdaq::setSi570IsConfigured() {
}*/

bool Bdaq::waitForPllLock(uint timeout) {
	logger->info("Waiting for PLL lock...");
	uint times = 0;
	bool locked = false;
	while (times < timeout && locked == false) {
		locked = rx.at(0).getPllLocked() && rx.at(1).getPllLocked() &&
		         rx.at(2).getPllLocked() && rx.at(3).getPllLocked() &&
				 rx.at(4).getPllLocked() && rx.at(5).getPllLocked() &&
				 rx.at(6).getPllLocked();
		++times;
	}
	if (locked) {
		logger->info("PLL locked!");
		return true;
	}
	else {
		logger->critical("Timeout while waiting for PLL to lock");
		exit(-1);
	}
}

void Bdaq::setupAurora() {
	if (boardOptionsMap[dv.boardOptions] == "640Mbps") {
		logger->info("Aurora receiver running at 640 Mb/s");
		cmd.setBypassMode(false);
	} else {
		logger->info("Aurora receiver running at 1.28 Gb/s");
		cmd.setBypassMode(false);
	}
}

void Bdaq::setChipTypeRD53A() {
	cmd.setChipType(0);
}

void Bdaq::setChipTypeITkPixV1() {
	cmd.setChipType(1);
}

void Bdaq::enableAutoSync() {
	cmd.setAutoSync(true);
}

void Bdaq::disableAutoSync() {
	cmd.setAutoSync(false);
}

void Bdaq::setMonitorFilter(BdaqAuroraRx::userkFilterMode mode) {
	for (uint i=0;i<7;++i) {
		rx.at(i).setUserKfilterMask(1, 0x00); // Only allow register data frames
		rx.at(i).setUserKfilterMask(2, 0x01); // Only allow register data frames
		rx.at(i).setUserKfilterMask(3, 0x02); // Only allow register data frames
		rx.at(i).setUserkFilterMode(mode);
	}
}
