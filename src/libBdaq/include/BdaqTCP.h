#ifndef BDAQTCP_H
#define BDAQTCP_H

// #################################
// # Author: Rafael Gama
// # Email: rafael.gama at cern.ch
// # Project: Yarr
// # Description: BDAQ SiTCP (FPGA Ethernet Module) TCP section interface.
// #              More information: http://sitcp.bbtech.co.jp
// # Comment: Should provide a similar functionality to basil/TL/SiTCP.py but
// #          was not based on this code.
// ################################

#include <boost/asio.hpp>

class BdaqTCP {
	public:
        BdaqTCP() : tcp(ios) {}
		~BdaqTCP() {
            tcp.close();
        }   

        void connect(std::string ipAdd = "192.168.10.12", int tcpPort = 24) {            
			boost::asio::ip::tcp::endpoint tcpEndpoint(
                boost::asio::ip::address::from_string(ipAdd), tcpPort);

			tcp.connect(tcpEndpoint);
                        
                        boost::asio::socket_base::receive_buffer_size option(10000000);
                        tcp.set_option(option);
                 
        }

        void read(std::vector<uint8_t>& buffer) {
            getData(buffer);
        }

        std::size_t getSize();
        void getData(std::vector<uint8_t>& buffer);
        void flush();

	private:
        boost::asio::io_service ios;
        boost::asio::ip::tcp::socket tcp;

};

#endif
