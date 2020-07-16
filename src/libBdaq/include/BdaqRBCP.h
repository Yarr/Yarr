#ifndef BDAQRBCP_H
#define BDAQRBCP_H

// #################################
// # Author: Rafael Gama
// # Email: rafael.gama at cern.ch
// # Project: Yarr
// # Description: BDAQ SiTCP (FPGA Ethernet Module) Remote Bus Control Protocol 
// #              (RBCP) interface. More information: http://sitcp.bbtech.co.jp
// # Comment: Should provide a similar functionality to basil/TL/SiTCP.py but
// #          was not based on this code.
// ################################

#include <boost/asio.hpp>

class BdaqRBCP {
	public:
        BdaqRBCP() : udp(ios) {}
		~BdaqRBCP() {
            udp.close();
        }   
        
        void connect(std::string ipAdd = "192.168.10.12", int udpPort = 4660) {
            boost::asio::ip::udp::endpoint udpEndpoint(
                boost::asio::ip::address::from_string(ipAdd), udpPort);
            udp.connect(udpEndpoint);
        }

        void read(int addr, std::vector<uint8_t>& data, int length) {
            rbcpRead(addr, data, length);
        }

        void write(int addr, const std::vector<uint8_t>& data) {
            rbcpWrite(addr, data);
        }
        
	private:
	    boost::asio::io_service ios;
	    boost::asio::ip::udp::socket udp;

        void rbcpRead(int addr, std::vector<uint8_t>& data, int length);
        void rbcpWrite(int addr, const std::vector<uint8_t>& data);
        int rbcpPacketRead(uint16_t addr, std::vector<uint8_t>& data, uint8_t length);
        int rbcpPacketWrite(uint16_t addr, const std::vector<uint8_t>& data, uint8_t length);
        int rbcpCheckAck(std::vector<uint8_t>& ackPacket, unsigned char pktId);
        void rbcpErrorHandler(std::string id, uint16_t addr, int code);
};

#endif
