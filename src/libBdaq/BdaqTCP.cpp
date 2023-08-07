// #################################
// # Author: Rafael Gama
// # Email: rafael.gama at cern.ch
// # Project: Yarr
// # Description: BDAQ SiTCP (FPGA Ethernet Module) section interface. 
// #              More information: http://sitcp.bbtech.co.jp
// # Comment: Should provide a similar functionality to basil/TL/SiTCP.py but
// #          was not based on this code.
// ################################

#include "BdaqTCP.h"

std::size_t BdaqTCP::getSize() {
    return tcp.available(); //in bytes
}

//The number of read bytes is determined by buffer.size()
void BdaqTCP::getData(std::vector<uint8_t>& buffer) {
    tcp.receive(boost::asio::buffer(buffer));
}

void BdaqTCP::flush() {
    std::size_t size = getSize();
    if (size != 0) {
        std::vector<uint8_t> buf(size);
        getData(buf);
    }    
}
