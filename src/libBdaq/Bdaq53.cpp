#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#include <math.h>

#include "Bdaq53.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("Bdaq53");
}

Bdaq53::Bdaq53() : 
	auroraRx(rbcp), 
	i2c(rbcp),
	si570(i2c),
	cmd(rbcp),
	fifo(tcp),
	bdaqControl(rbcp) {}

void Bdaq53::initialize(bdaqConfig c) {
	rbcp.connect(c.ipAddr, c.udpPort);
	auroraRx.setBase(c.rxAddr);
	auroraRx.checkVersion();
	i2c.setBase(c.i2cAddr);
	i2c.checkVersion();
	i2c.init();
	cmd.setBase(c.cmdAddr);
	cmd.checkVersion();
	cmd.init();
	tcp.connect(c.ipAddr, c.tcpPort);

	bdaqControl.setBase(c.controlAddr);
	bdaqControl.init(24, uint(pow(2.0, 24.0) - 1));
	bdaqControl.setData(0);

	dv = getDaqVersion(); // getDaqVersion must be called after auroraRx.setBase()
	if (VERSION != dv.fwVersion) {
		std::string error = "Firmware version " + dv.fwVersion + 
		" is different than software version " + VERSION + "! Please update.";
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
	//Check if Si570 is configured. If not, configure it.
 	//if (auroraRx.getSi570IsConfigured() == false) {
		cmd.setOutputEn(false);
		auroraRx.reset();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		si570.init(0xBA, 160.0); //0xBA is the Si570 i2c slave address, 160 MHz.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		cmd.setOutputEn(true);
		//auroraRx.setSi570IsConfigured(true);
	/*} else
		logger->info("Si570 oscillator is already configured");*/
	//Reset cmd encoder
	cmd.reset();
	//Setting BdaqDriver to RD53A
	setChipTypeRD53A(); // Gotta move it to YARR code
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//Configure the Aurora link
	waitForPllLock();
	setupAurora();
}

daqVersion Bdaq53::getDaqVersion() {
	std::vector<uint8_t> buf(2);
	daqVersion dv;
	//Firmware version
	rbcp.read(0x0, buf, 2);
	dv.fwVersion = std::to_string(buf.at(1)) + "." + std::to_string(buf.at(0));
	//Board version
	rbcp.read(0x2, buf, 2);
	dv.boardVersion = hwMap[buf.at(0)];
	//Number of RX channels
	rbcp.read(0x3, buf, 1);
	dv.numRxChannels = buf.at(0);
	//Board options
	rbcp.read(0x4, buf, 1);
	dv.boardOptions = buf.at(0);
	//Connector Version
	rbcp.read(0x5, buf, 2);
	dv.connectorVersion = hwConMap[buf.at(0)];
	return dv;
}

/*void Bdaq53::setSi570IsConfigured() {
}*/

bool Bdaq53::waitForPllLock(uint timeout) {
	logger->info("Waiting for PLL lock...");
	uint times = 0;
	while (times < timeout && auroraRx.getPllLocked() == false)
		++times;
	if (auroraRx.getPllLocked()) {
		logger->info("PLL locked!");
		return true;
	}
	else {
		logger->critical("Timeout while waiting for PLL to lock");
		exit(-1);
	}
}

void Bdaq53::setupAurora() {
	if (boardOptionsMap[dv.boardOptions] == "640Mbps") {
		logger->info("Aurora receiver running at 640 Mb/s");
		cmd.setBypassMode(false);
	} else {
		logger->info("Aurora receiver running at 1.28 Gb/s");
		cmd.setBypassMode(false);
	}
}

void Bdaq53::setChipTypeRD53A() {
	cmd.setChipType(0);
}

void Bdaq53::setChipTypeITkPixV1() {
	cmd.setChipType(1);
}

void Bdaq53::enableAutoSync() {
	cmd.setAutoSync(true);
}

void Bdaq53::disableAutoSync() {
	cmd.setAutoSync(false);
}

void Bdaq53::setMonitorFilter(BdaqAuroraRx::userkFilterMode mode) {
	auroraRx.setUserKfilterMask(1, 0x00); // Only allow register data frames
	auroraRx.setUserKfilterMask(2, 0x01); // Only allow register data frames
	auroraRx.setUserKfilterMask(3, 0x02); // Only allow register data frames
	auroraRx.setUserkFilterMode(mode);
}