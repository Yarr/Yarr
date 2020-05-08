#include <iostream>
#include <vector>

#include "Bdaq53.h"

Bdaq53::Bdaq53() : 
	auroraRx(rbcp), 
	i2c(rbcp),
	si570(i2c),
	cmd(rbcp),
	fifo(tcp),
	readout(fifo),
	dpControl(rbcp) {}

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
	dv = getDaqVersion(); // getDaqVersion must be called after auroraRx.setBase()
	if (VERSION != dv.fwVersion) {
		std::string error = "Firmware version " + dv.fwVersion + 
		" is different than software version " + VERSION + "! Please update.";
		throw std::runtime_error(error);
	}
	if (dv.boardVersion != "BDAQ53")
	{
		std::string error = "Only BDAQ53 board is supported in \"bdaq\" "
			"controller mode.";
		throw std::runtime_error(error);
	}
	std::cout << "\033[1;32mFound board " + dv.boardVersion + " with " + 
		dv.connectorVersion + " running firmware version " + 
		dv.fwVersion + "\033[0m" << std::endl;
	std::cout << "Found " << dv.rxChannels << " Aurora receiver channel(s) with " <<
		dv.rxLanes << " lane(s)" << std::endl;
	//Check if Si570 is configured. If not, configure it.
 	if (auroraRx.getSi570IsConfigured() == false) {
		cmd.setOutputEn(false);
		auroraRx.reset();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		si570.init(0xBA, 160.0); //0xBA is the Si570 i2c slave address, 160 MHz.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		cmd.setOutputEn(true);
		auroraRx.setSi570IsConfigured(true);
	} else
		std::cout << "Si570 oscillator is already configured" << std::endl;
	//Reset cmd encoder
	cmd.reset(); 
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
	dv.boardVersion = hwMap[buf.at(0) + (buf.at(1) << 8)];
	//Board options
	rbcp.read(0x4, buf, 1);
	dv.boardOptions = buf.at(0);
	//Connector Version
	rbcp.read(0x5, buf, 2);
	dv.connectorVersion = hwConMap[buf.at(0) + (buf.at(1) << 8)];
	//RX lanes and RX channels
	rxConfig rxc = auroraRx.getRxConfig();
	dv.rxLanes = rxc.rxLanes;
	dv.rxChannels = rxc.rxChannels;
	return dv;
}

bool Bdaq53::waitForPllLock(uint timeout) {
	std::cout << "Waiting for PLL lock..." << std::endl;
	uint times = 0;
	while (times < timeout && auroraRx.getPllLocked() == false)
		++times;
	if (auroraRx.getPllLocked()) {
		std::cout << "PLL locked!" << std::endl;
		return true;
	}
	else 
		throw std::runtime_error("Timeout while waiting for PLL to lock.");
}

void Bdaq53::setupAurora() {
	if (boardOptionsMap[dv.boardOptions] == "640Mbps") {
		std::cout << "Aurora receiver running at 640 Mb/s" << std::endl;
		cmd.setBypassMode(false);
	} else {
		std::cout << "Aurora receiver running at 1.28 Gb/s" << std::endl;
		cmd.setBypassMode(false);
	}
}
