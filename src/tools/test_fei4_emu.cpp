
#include "Fei4Emu.h"

int main(int argc, const char** argv) {

  Fei4Emu fei4Emu;

  std::vector<uint8_t> fei4Cmds;
  auto pushCmd = [&](uint32_t cmd) {
    for( auto offset : { 24, 16, 8, 0 } )
      fei4Cmds.push_back( (cmd & (0xFF << offset)) >> offset );
  };


  pushCmd(0x161); // BCR
  pushCmd(0x162); // ECR
  pushCmd(0x1d0); // L1A

#if 0
  for( uint8_t cmdByte : fei4Cmds )
    std::cout << std::hex << (int)cmdByte << std::dec << std::endl;
#endif

  fei4Emu.decodeCommand((uint8_t*)&fei4Cmds[0], fei4Cmds.size());

  for(auto& d : fei4Emu.getFeStream())
    std::cout << std::hex << d << std::endl;
  std::cout << std::dec;

  return EXIT_SUCCESS;
}
