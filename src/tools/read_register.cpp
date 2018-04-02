#include <unistd.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "SpecController.h"
#include "json.hpp"

#include "Fei4.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

void printHelp();

int main(int argc, char *argv[]) {

  std::string cConfigPath = "";
  bool doCfg = false;
  bool verbose = false;

  int c;
  while ((c = getopt(argc, argv, "hivc:")) != -1) {
    switch (c) {
    case 'h':
      printHelp();
      return 0;
      break;
    case 'c':
      cConfigPath = std::string(optarg);
      break;
    case 'i':
      doCfg = true;
      break;
    case 'v':
      verbose = true;
      break;
    case '?':
      std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
      return -1;
    default:
      std::cerr << "-> Error while parsing command line parameters!" << std::endl;
      return -1;
    }
  }

  if (cConfigPath == "") {
    std::cerr << "Error: no config files given, please specify config file name under -c option, even if file does not exist!" << std::endl;
    return -1;
  }

  SpecController mySpec;
  mySpec.init(0);

  mySpec.setCmdEnable(0x1);

  Fei4 fe(&mySpec, 0, 0);
  Fei4 dummy(&mySpec, 8, 8);

  // Read config file
  std::fstream cfgFile;
  json input;
  cfgFile.open(cConfigPath, std::ios::in);
  // json input;
  //std::cout << input["FE-I4B"]["GlobalConfig"].dump(4) << std::endl;
  cfgFile >> input;
  fe.fromFileJson(input);

  std::cout << "------------------------------------------" << std::endl;
  std::cout << std::endl << "check registers" << std::endl;
  std::cout << "config file: " << cConfigPath << std::endl << std::endl;
  std::cout << "------------------------------------------" << std::endl;


  fe.setRunMode(false);
  if(doCfg) {
    fe.configure();
    while(!mySpec.isCmdEmpty()) {}
  }
  mySpec.setRxEnable(0x1);


  //----------------------------------------------------------------
  // json file from chip to be compared with config file
  json replica;


  //----------------------------------------------------------------
  // check Global registers
  for (unsigned i=0; i< fe.numRegs; i++) {
    if(verbose)
      std::cout << "Reading reg: " << i << std::endl;
    fe.readRegister(i);
    usleep(1000);
    while(!mySpec.isCmdEmpty()) {}
    dummy.cfg[i] = 0xDEAD;
    RawData *data = mySpec.readData();
    while(!mySpec.isCmdEmpty()) {}
    while (data != NULL) {
      if (data != NULL) {
	uint32_t greg = 99;
	uint32_t value = 0;
	for (unsigned j=0; j<data->words;j++) {
	  if(verbose)
	    std::cout << "[" << j << "] = 0x" << std::hex << data->buf[j] << std::dec << std::endl;
	  if ((data->buf[j] & 0x00FF0000) == 0x00ea0000) {
	    greg = (data->buf[j] & 0x0000FFFF);
	  }
	  if ((data->buf[j] & 0x00FF0000) == 0x00ec0000) {
	    value = (data->buf[j] & 0x0000FFFF);
	  }
	  dummy.cfg[greg] = value;
	}
      }
      delete data;
      data = mySpec.readData();
      while(!mySpec.isCmdEmpty()) {}
    }
  }
  dummy.toFileJson(replica);

  if(verbose)
    std::cout << replica["FE-I4B"]["GlobalConfig"].dump(4) << std::endl;


  //----------------------------------------------------------------
  // check Pixel registers
  const unsigned nRow = Fei4PixelCfg::n_Row;
  const unsigned nCol = Fei4PixelCfg::n_Col;

  bool bits[Fei4PixelCfg::n_Bits][nRow][nCol];

  // unsigned dc = 0; // 0-39
  for (unsigned dc=0; dc<Fei4PixelCfg::n_DC; dc++) {
    // unsigned latch = 2; // 0-12
    // // [0]: Enable
    // // [1:5] TDAC value [1]=MSB
    // // [6] Large injection capacitor
    // // [7] Small injection capacitor
    // // [8] Imon and Hitbus out
    // // [9:12] FDAC value [12]=MSB
    for (unsigned latch=0; latch<Fei4PixelCfg::n_Bits; latch++) {

      if(verbose)
	std::cout << "dc, latch: " << dc << ", " << latch << std::endl;

      fe.readPixelRegister(dc, (0b1 << latch));
      usleep(1000);
      while(!mySpec.isCmdEmpty()) {}

      int column = -1;
      int row = -1;
      int r = -1;

      RawData *data = mySpec.readData();
      while(!mySpec.isCmdEmpty()) {}
      while (data != NULL) {
	if (data != NULL) {
	  for (unsigned j=0; j<data->words;j++) {

	    if ((data->buf[j] & 0x00FF0000) == 0x00ef0000) {
	      j = data->words-1;
	    }

	    uint32_t value = 0;
	    uint32_t addr = 0;

	    // addr
	    if ((data->buf[j] & 0x00FF0000) == 0x00ea0000) {
	      addr = (data->buf[j] & 0x0000FFFF);
	      if(verbose)
		std::cout << "addr: " << std::hex << addr << std::dec << std::endl;
	      r = ((addr & 0xFF0) >> 0x4);
	      if(r <= 20) column = 2*dc + 2;
	      else column = 2*dc + 1;
	    }

	    // value
	    if ((data->buf[j] & 0x00FF0000) == 0x00ec0000) {
	      value = (data->buf[j] & 0x0000FFFF);
	      // returned bit is inverted
	      value ^= 0xFFFF;
	      if(verbose)
		std::cout << "value: " << std::hex << value << std::dec << std::endl;

	      for(int i=0; i<16; i++) {
		bool b = ((value >> i) & 0b1);
		// std::cout << b << " ";
		if(r <= 20) row = 16*(20-r) + i + 1;
		else row = 16*(r-21) + (16 - i);
		// std::cout << "[column, row]: [" << column << ", " << row << "] = " << b << std::endl;
		bits[latch][row-1][column-1] = b;
	      }
	      // std::cout << std::endl;
	    }

	    // if(verbose)
	    //   std::cout << "[" << j << "] = 0x" << std::hex << data->buf[j] << std::dec << std::endl;
	  }
	}
	delete data;
	data = mySpec.readData();
	while(!mySpec.isCmdEmpty()) {}
      }

    }
  }


  for (unsigned row=1; row<=nRow; row++) {
    for (unsigned col=1; col<=nCol; col++) {

      dummy.setEn(  col, row, bits[0][row-1][col-1]);
      dummy.setTDAC(col, row, 16*bits[1][row-1][col-1]
		    + 8*bits[2][row-1][col-1]
		    + 4*bits[3][row-1][col-1]
		    + 2*bits[4][row-1][col-1]
		    +   bits[5][row-1][col-1]);
      dummy.setLCap(  col, row, bits[6][row-1][col-1]);
      dummy.setSCap(  col, row, bits[7][row-1][col-1]);
      dummy.setHitbus(col, row, bits[8][row-1][col-1]);
      dummy.setFDAC(  col, row, bits[9][row-1][col-1]
		      + 2*bits[10][row-1][col-1]
		      + 4*bits[11][row-1][col-1]
		      + 8*bits[12][row-1][col-1]);

    }
  }

  dummy.toFileJson(replica);


  //----------------------------------------------------------------
  // config again to avoid changing global registers
  fe.dummyCmd();
  while(!mySpec.isCmdEmpty()) {}


  //----------------------------------------------------------------
  // print comparison results

  if(input["FE-I4B"]["GlobalConfig"] == replica["FE-I4B"]["GlobalConfig"]) {
    std::cout << "No difference from Global config :)" << std::endl;
  }
  else {
    std::cout << std::endl << "config name : read from config file, read from chip" << std::endl << std::endl;

    for (json::iterator it_input = input["FE-I4B"]["GlobalConfig"].begin(); it_input != input["FE-I4B"]["GlobalConfig"].end(); ++it_input) {
      if(it_input.value() != replica["FE-I4B"]["GlobalConfig"][it_input.key()]) {
	std::cout << it_input.key() << " : " << it_input.value() << ", " << replica["FE-I4B"]["GlobalConfig"][it_input.key()] << std::endl;
      }
    }
    std::cout << std::endl;
  }


  if(input["FE-I4B"]["PixelConfig"] == replica["FE-I4B"]["PixelConfig"]) {
    std::cout << "No difference from Pixel config :)" << std::endl;
  }
  else {

    const int npreg = 6;
    std::string pregname[npreg] = {"Enable", "Hitbus", "TDAC", "LCap", "SCap", "FDAC"};

    for (unsigned row=1; row<=nRow; row++) {
      // for (unsigned col=1; col<=nCol; col++) {

      if(replica["FE-I4B"]["PixelConfig"][row-1]["Row"] != input["FE-I4B"]["PixelConfig"][row-1]["Row"])
	std::cout << row << ", Row" << " : " << replica["FE-I4B"]["PixelConfig"][row-1]["Row"] << ", " << input["FE-I4B"]["PixelConfig"][row-1]["Row"] << std::endl;

      for (int p=0; p<npreg; p++) {
    	if(replica["FE-I4B"]["PixelConfig"][row-1][pregname[p]] != input["FE-I4B"]["PixelConfig"][row-1][pregname[p]]) {
    	  for (unsigned col=1; col<=nCol; col++) {
    	    if(replica["FE-I4B"]["PixelConfig"][row-1][pregname[p]][col-1] != input["FE-I4B"]["PixelConfig"][row-1][pregname[p]][col-1]) {

    	      std::cout << row << ", " << col << ", " << pregname[p] << " : " << replica["FE-I4B"]["PixelConfig"][row-1][pregname[p]][col-1] << ", " << input["FE-I4B"]["PixelConfig"][row-1][pregname[p]][col-1] << std::endl;
    	    }
    	  }
    	}
      }
    }
    std::cout << std::endl;
  }

  std::cout << std::endl << "finish" << std::endl << std::endl;
  std::cout << "------------------------------------------" << std::endl;


  return 0;
}

void printHelp() {
  std::cout << "Help:" << std::endl;
  std::cout << " -c <cfg1.json>: Provide chip configuration, canNOT take multiple arguments." << std::endl;
  std::cout << " -i: Configure chip." << std::endl;
  std::cout << " -v: Verbose mode" << std::endl;
}
