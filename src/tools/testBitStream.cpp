#include "BitStream.h"

#include <iostream>
#include <iomanip>

using namespace std;

int main(int argc, char** argv){

  cout << "Create BitStream" << endl;
  BitStream * cmd = new BitStream();

  cout << "Test 2" << endl;
  cout << "Add 10110" << endl;
  cmd->Add(5,0x16);
  cout << cmd->ToString() << endl;
  cout << "Add 1000" << endl;
  cmd->Add(4,0x8);
  cout << cmd->ToString() << endl;
  cout << "Add 0001" << endl;
  cmd->Add(4,0x1);
  cout << cmd->ToString() << endl;
  cout << "Add 1000" << endl;
  cmd->Add(4,0x8);
  cout << cmd->ToString() << endl;
  cout << "Add 011011" << endl;
  cmd->Add(6,0x1B);
  cout << cmd->ToString() << endl;
  
  cmd->Clear();
  
  cout << "Test 2" << endl;
  cmd->Add(5,0x16).Add(4,0x8).Add(4,0x1).Add(4,0x8).Add(6,0x1B).Add(16,0x0000);
  cout << "0b" << cmd->ToString() << endl;
  cout << "0x" << cmd->ToHexString() << endl;

  cmd->Pack();
  for(uint32_t i=0;i<cmd->GetSize();i++){
	cout << hex << "0x" << cmd->GetWord(i) << endl;
  }

  return 0;
}
