#include "NetioHW/NetioRxCore.h"
#include "NetioHW/NetioTxCore.h"
#include "BitStream.h"
#include "RawData.h"
#include <cmdl/cmdargs.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <time.h>

using namespace std;

int main(int argc, char** argv){

  CmdArgInt clink ('e',(const char*)"link", (const char*)"link",(const char*)"link");
  CmdArgStr cfile ('f',(const char*)"file", (const char*)"file",(const char*)"file");
  CmdLine cmdl(*argv,&clink,&cfile, NULL);
  CmdArgvIter arg_iter(argc-1,argv+1);
  cfile = "";
  clink = 0;
  cmdl.parse(arg_iter);

  cout << "Create RxCore" << endl;
  RxCore * rxcore = new NetioRxCore();

  cout << "Enable rx channel: " << clink << endl;
  rxcore->enableChannel(clink);

  cout << "Create TxCore" << endl;
  TxCore * txcore = new NetioTxCore();

  cout << "Enable tx channel: " << clink << endl;
  txcore->enableChannel(clink);
  txcore->setTrigChannel(clink,true);


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
	  RawData * data = rxcore->readData(clink);
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
	
  

  delete rxcore;
  delete txcore;
  cout << "Have a nice day" << endl;
  return 0;

}
