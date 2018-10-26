#include "AllHwControllers.h"
#include "BitStream.h"
#include "RawData.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>

#include <unistd.h>

using namespace std;

int main(int argc, char** argv){
  int clink = 0;
  std::string cfile = "";

  int c;
  while ((c = getopt(argc, argv, "e:f:")) != -1) {
    switch(c) {
    case 'e':
      clink = stoi(std::string(optarg));
      break;
    case 'f':
      cfile = std::string(optarg);
      break;

    default:
      std::cerr << "-> Error while parsing command line parameters!" << std::endl;
      return -1;
    }
  }

  std::unique_ptr<HwController> hw = StdDict::getHwController("Netio");

  cout << "Create NetIO" << endl;

  RxCore * rxcore = &*hw;;

  cout << "Enable rx channel: " << clink << endl;
  rxcore->setRxEnable(1<<clink);

  TxCore * txcore = &*hw;

  cout << "Enable tx channel: " << clink << endl;
  txcore->setCmdEnable(1<<clink);
  //  txcore->setTrigChannel(clink,true);

  BitStream cmd;

  string line;
  ifstream fr(cfile);
  std::cout << "Before parsing file...\n";
  while(getline(fr, line)) {
       
	//catch symbols in the line
	std::cout << "line: " << line << "\n";

        stringstream ss(line);
        unsigned sleep=0;
        std::string hexStr("");
        ss >> hexStr;
        if (hexStr=="0x0"){
          ss>>sleep;
          //std::cout << "I should sleep: " << sleep << std::endl;
          std::this_thread::sleep_for(std::chrono::microseconds(sleep));
        } else {
          std::cout << "I should call: " << hexStr << std::endl;
        }

        hexStr.erase(0,2);
		cmd.Clear();
		cmd.FromHex(hexStr);
		cmd.Pack();

        std::cout << "WAS: " << hexStr << endl;
        std::cout << "HEX: " << cmd.ToHexString() << endl;
        std::cout << "STR: " << cmd.ToString() << endl;

	for(uint32_t i=0; i<cmd.GetSize();i++){
	  txcore->writeFifo(cmd.GetWord(i));
	}
	txcore->releaseFifo();
  }

  
  //readout the data
    cout << "Read-out" << endl;
	RawDataContainer datav;
	time_t endtime;
    time_t curtime = time(NULL);
    endtime = curtime + 1;

    while (curtime < endtime){
	  RawData * data = rxcore->readData();
          // Check which place it came from? (clink?)
	  if(data==NULL){std::this_thread::sleep_for(std::chrono::milliseconds(100));cout<<"."<<flush;continue;}
	  for(uint32_t i=0;i<data->words;i++){
		cout << hex << data->buf[i] << dec << endl;
	  }
	  datav.add(data);
	  cout << "Event " << datav.size() << endl;
	  curtime = time(NULL);
	}
	
	cout << "List of L1A received" << endl;
	for(uint32_t i=0; i<datav.size(); i++){
	  cout << "L1A: " << ((datav.buf[i][0]>>10)&0x1F) << " " 
		   << "BCID: "<< (datav.buf[i][0]&0x3FF) << endl;
	}
	
  cout << "Have a nice day" << endl;
  return 0;
}
