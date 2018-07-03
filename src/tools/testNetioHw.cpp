#include "NetioTxCore.h"
#include "NetioRxCore.h"
#include "AllHwControllers.h"
#include "BitStream.h"
#include "RawData.h"
#include "HwController.h"

#include <iostream>
#include <map>
#include <iomanip>
#include <sstream>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

using namespace std;

static void printHelp();

uint32_t readConfig(TxCore * txcore, RxCore * rxcore, uint32_t addr){
  uint32_t rdreg = 0x5A0600 | addr;
  uint32_t vlreg = 0;
  bool ok=false;
  do{
    txcore->writeFifo(rdreg);
    txcore->releaseFifo();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while(1) {
      RawData * data = rxcore->readData();
      if(data == nullptr) {
        cout << "Timeout." << endl;
        continue;
      }

      for(uint32_t i=0;i<data->words;i++){
        uint32_t hdr=(data->buf[i]>>16)&0xFF;
        uint32_t val=(data->buf[i]&0xFFFF);
        if(hdr==0xEA && val!=addr){
          cout << "Something went wrong1: " << hex 
               << " hdr=" << hdr
               << " val=" << val
               << dec << endl;
        }
        else if(hdr==0xEC){vlreg=val; ok=true;}
        else{continue;}
      }
    }
  }while(!ok);
  return vlreg;
}

void printConfig(uint32_t addr, uint32_t value){
  cout << setfill(' ') << setw(2) << addr << ": 0x" 
	   << setfill('0') << setw(4) << hex << value << dec << endl;
}

int main(int argc, char** argv){
  std::string chost = "localhost";
  int cetx = 0;
  int cerx = 0;
  int chid = 8;
  int cptx = 12340;
  int cprx = 12345;
  bool verbose = false;

  int c;
  while ((c = getopt(argc, argv, "H:T:R:t:r:c:vh")) != -1) {
    switch(c) {
    case 'H': chost = std::string(optarg); break;
    case 'T': cptx = atoi(optarg); break;
    case 'R': cprx = atoi(optarg); break;
    case 't': cetx = atoi(optarg); break;
    case 'r': cerx = atoi(optarg); break;
    case 'c': chid = atoi(optarg); break;
    case 'v': verbose = true; break;
    case 'h': printHelp(); return 0;
    default: std::cout << "Error parsing command line\n"; return -1;
    }
  }

  json j;
  j["NetIO"]["host"] = chost;
  j["NetIO"]["txport"] = cptx;
  j["NetIO"]["rxport"] = cprx;

  cout << "Create NetIO with options\n";
  cout << j << endl;

  std::unique_ptr<HwController> hw = StdDict::getHwController("Netio");
  // new NetioController;
  hw->loadConfig(j);

  RxCore * rxcore = static_cast<RxCore*>(&*hw);
  //  rxcore->setVerbose(verbose);

  cout << "Enable rx e-link: " << cerx << endl;
  rxcore->setRxEnable(1<<cerx);

  cout << "Create TxCore" << endl;
  TxCore * txcore = static_cast<TxCore*>(&*hw);
  //txcore->setVerbose(verbose);

  cout << "Enable tx e-link: " << cetx << endl;
  txcore->setCmdEnable(1<<cetx);
  txcore->setTrigEnable(1<<cetx);
  
  cout << "Enable CFG" << endl;
  txcore->writeFifo(0x5A2A07);
  txcore->releaseFifo();

  cout << "Configure the chip" << endl;
  /*
  uint32_t cfg[] = {0x0000,0x0000,0x0800,0x4601, //0-3
					0x0040,0xd405,0x00d4,0x6914, //4-7
					0xf258,0x00aa,0x784c,0x56d4, //8-11
					0x6200,0x0000,0xd526,0x1a96, //12-15: 1400, 
					0x0038,0x00ab,0x32ff,0xffff, //16-19: 0038,00ab,32ff,0640
					0x1369,0x0000,0x0000,0x0000, //20-23: 1369
					0x0000,0xc900,0x0058,0x8000, //24-27: 0000,c900,0058,8200
					0x8206,0x0007,0x0000,0xf400}; //28-31:
  */
  uint32_t cfg[] = {0x0000,0x0000,0x1800,0x0000, //0-3
					0x0000,0xd4ff,0x7cd4,0xff58, //4-7
					0xf258,0x00aa,0x4cb0,0x56d4, //8-11
					0x2800,0x0000,0xabb7,0x1a96, //12-15: 1400, 
					0x0038,0x00ab,0x00ff,0x6000, //16-19: 0038,00ab,32ff,0640
					0xffff,0x0584,0x0000,0x0000, //20-23: 1369
					0x0000,0xd200,0x0002,0x8000, //24-27: 0000,c900,0058,8200
					0x8206,0x0007,0x0000,0xf400}; //28-31:

  BitStream bs;
  for(int32_t i=0;i<32;i++){
    bs.Add(5,0x16).Add(4,0x8).Add(4,0x2).Add(4,chid).Add(6,i).Add(16,cfg[i]);
    bs.Pack();
    for(uint32_t j=0;j<bs.GetSize();j++){
      txcore->writeFifo(bs.GetWord(j));
    }
    cout << setw(2) << i << ": 0x" << hex << setw(4) << setfill('0') << cfg[i] << dec << endl;
    txcore->releaseFifo();
    bs.Clear();
  }
  /*
  cout << "One trigger at a time" << endl;
  txcore->writeFifo(0x5A0A00| 2);
  //txcore->writeFifo(0x000000| 16 << (12+16));
  txcore->writeFifo(0x08000000);  
  txcore->releaseFifo(); 
  */
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   
  map<uint32_t,uint32_t> registers;
  cout << "Read configuration registers" << endl;  
  for(uint32_t addr=2;addr<32;addr++){
    registers[addr]=readConfig(txcore,rxcore,addr);
  }
  
  map<uint32_t,uint32_t>::iterator it=registers.begin();
  for(;it!=registers.end();it++){
    printConfig(it->first,it->second);
  }
  
 /* 
  //Clear the shift register 
  cout << "Clear the shift register" << endl;
  
  //Write GR22 
  txcore->writeFifo(0x0000005A);
  //txcore->writeFifo(0x0A1600FC); //write to loopback
  txcore->writeFifo(0x0A160014);
  txcore->releaseFifo();
  
  //Write GR13 
  txcore->writeFifo(0x0000005A);
  //txcore->writeFifo(0x0A0D3FFE);
  txcore->writeFifo(0x0);
  txcore->releaseFifo();

  //Write GR21 
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A150000);
  txcore->releaseFifo();

  //Write GR27
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A1B8000);
  txcore->releaseFifo();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  //Pulse 
  cout << "Pulse" << endl;
  txcore->writeFifo(0x5A260A);
  txcore->releaseFifo();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  //try to write to the shift registers

  //Write GR22 
  txcore->writeFifo(0x0000005A);
  //txcore->writeFifo(0x0A1600FC); //write to loopback
  txcore->writeFifo(0x0A160014);
  txcore->releaseFifo();
  
  //Write GR13 
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A0D3FFE);
  txcore->releaseFifo();

  //Write GR21 
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A150000);
  txcore->releaseFifo();
  
  //Write GR27
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A1B8200);
  txcore->releaseFifo();

  */
/*
  //Write GR22 
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A160014);
  txcore->releaseFifo();

  //Write GR13 
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A0DC001);
  txcore->releaseFifo();

  //Write GR21 
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A150000);
  txcore->releaseFifo();

  //Write GR27
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A1BC000);
  txcore->releaseFifo();

  //Pulse 
  cout << "Global Pulse" << endl;
  txcore->writeFifo(0x5A260A);
  txcore->releaseFifo();
 */ 
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  cout << "Read configuration registers" << endl;  
  registers[13]=readConfig(txcore,rxcore,13);
  registers[21]=readConfig(txcore,rxcore,21);
  registers[22]=readConfig(txcore,rxcore,22);
  registers[27]=readConfig(txcore,rxcore,27);
  printConfig(13,registers[13]);
  printConfig(21,registers[21]);
  printConfig(22,registers[22]);
  printConfig(27,registers[27]);
/*
  cout << "Readback the shift register" << endl;

  //Write GR13 
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A0D3FFE);
  txcore->releaseFifo();
  
  //Write GR27
  txcore->writeFifo(0x0000005A);
  txcore->writeFifo(0x0A1B8202);
  txcore->releaseFifo();
*/

  string var;
/*
  cout << "Perform Analog Injection" << endl;
  cout << "Press any key to continue..." << flush;
  string var;
  cin >> var;
  //Write GR22 : select double column 
  txcore->writeFifo("0000005A");
  txcore->writeFifo("0A160014");
  txcore->releaseFifo();
  //Write GR13 : set S0=S1=0 | Enable Large and Small Capacitors and Output
  txcore->writeFifo("0000005A");
  txcore->writeFifo("0A0D00C2");
  txcore->releaseFifo();
  //Write GR21 : set HitLd=0 
  txcore->writeFifo("0000005A");
  txcore->writeFifo("0A150000");
  txcore->releaseFifo();
  //WrFrontend: write shift register
  txcore->writeFifo("005A0A00");
  for(uint32_t i=0;i<21;i++){
	txcore->writeFifo("10101010");
  }
  txcore->releaseFifo();
  //Write GR27 : set LatchEn = 1
  txcore->writeFifo("0000005A");
  txcore->writeFifo("0A1B8004");
  txcore->releaseFifo();
  //Pulse 
  cout << "Global Pulse" << endl;
  txcore->writeFifo("5A260A");
  txcore->releaseFifo();
  //Write GR22 : select double column 
  txcore->writeFifo("0000005A");
  txcore->writeFifo("0A160014");
  txcore->releaseFifo();
  //Write GR27 : set LatchEn = 1
  txcore->writeFifo("0000005A");
  txcore->writeFifo("0A1B8010");
  txcore->releaseFifo();
  //Pulse 
  cout << "Global Pulse" << endl;
  txcore->writeFifo("5A260A");
  txcore->releaseFifo();
*/


  /*
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  cout << "Read configuration registers" << endl;  
  registers[13]=readConfig(txcore,rxcore,13);
  registers[21]=readConfig(txcore,rxcore,21);
  registers[22]=readConfig(txcore,rxcore,22);
  registers[27]=readConfig(txcore,rxcore,27);
  printConfig(13,registers[13]);
  printConfig(21,registers[21]);
  printConfig(22,registers[22]);
  printConfig(27,registers[27]);
  */

  cout << "Press any key to continue..." << flush;
  //string var;
  cin >> var;

  //WrFrontend: write shift register
  /*txcore->writeFifo("005A0A00");
  for(uint32_t i=0;i<21;i++){
	txcore->writeFifo("10101010");
  }
  txcore->releaseFifo();
  */
  
  /*cout << "Waiting for shift register to come back" << endl;
  RawData * rb=0;
  while(rb==0){
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	rb = rxcore->readData();
	if (rb==0) continue;
	for(uint32_t i=0;i<rb->words;i++){
	  cout << hex << rb->buf[i] << dec << endl;
	}
  }
  
  return 0;*/

  cout << "Enable DTM" << endl;
  txcore->writeFifo(0x5A2A38);
  txcore->releaseFifo();
  
  
  cout << "Configure the trigger" << endl;
  uint32_t ntriggers = 10;
  uint32_t trigwords[4] = {0,0xe8000000,0,0x00000164};
  txcore->setTrigWord(trigwords,4);
  txcore->setTrigFreq(1);
  txcore->setTrigCnt(ntriggers);
  txcore->setTrigConfig(INT_COUNT);
  txcore->setTrigEnable(1);

  cout << "Read-out" << endl;
  RawDataContainer datav;
  do{
	RawData * data = rxcore->readData();
	if(data==NULL){std::this_thread::sleep_for(std::chrono::milliseconds(100));cout<<"."<<flush;continue;}
	datav.add(data);
	cout << "Event " << datav.size() << endl;
  }while(datav.size()<ntriggers*16);
  
  cout << "List of L1A received" << endl;
  for(uint32_t i=0; i<datav.size(); i++){
	cout << "L1A: " << ((datav.buf[i][0]>>10)&0x1F) << " " 
		 << "BCID: "<< (datav.buf[i][0]&0x3FF) << endl;
  }
  
  cout << "Clean the house" << endl;
  // delete rxcore;
  // delete txcore;

  cout << "Have a nice day" << endl;
  return 0;
}

void printHelp() {
  std::cout << "Help:\n";
  std::cout << "  -H  host  server hostname (locahost)\n";
  std::cout << "  -T  txport  TX port number (12340)\n";
  std::cout << "  -R  rxport  RX port number (12345)\n";
  std::cout << "  -t  tx-elink  TX elink number (0)\n";
  std::cout << "  -r  rx-elink  RX elink number (0)\n";
  std::cout << "  -r  rx-elink  RX elink number (0)\n";
  std::cout << "  -c  chipid    Chip ID (8)\n";
  std::cout << "  -v      Set verbose mode\n";
}
