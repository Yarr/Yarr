// #################################
// # Author: Rafael Gama
// # Email: rafael.gama at cern.ch
// # Project: Yarr
// # Description: BDAQ SiTCP (FPGA Ethernet Module) Remote Bus Control Protocol 
// #              (RBCP) interface. More information: http://sitcp.bbtech.co.jp
// # Comment: Should provide a similar functionality to basil/TL/SiTCP.py but
// #          was not based on this code.
// ################################

#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <thread>

// RBCP constants
#define RBCP_VER         0xFF
#define RBCP_CMD_WR      0x80
#define RBCP_CMD_RD      0xC0
#define RBCP_HEADER_SIZE 8    // in bytes
#define RBCP_DATA_SIZE   0xFF // bytes
#define ACK_WAIT_COUNTER 200 //200  // in microseconds
#define MAX_TRIALS       4 //4
#define BYPASS_WRITE_ACK 0 //Bypass Write Acknowledgement. Experimental only, setting to 1 (Enabling) will make pretty much everything fail. :)
// RBCP ACK codes
#define NO_ACK          -1
#define ACK_SHORT       -2
#define ACK_ID_MISMATCH -3
#define ACK_SUCCESS     1000

#include "BdaqRBCP.h"

void BdaqRBCP::rbcpRead(int addr, std::vector<uint8_t>& data, int length) {
    int nDiv = length / RBCP_DATA_SIZE;
    int nMod = length % RBCP_DATA_SIZE;
    data.clear();
    data.reserve(length);
    int i;
    for (i = 0; i < nDiv; ++i) {
        int busAddr = addr + (RBCP_DATA_SIZE * i); //calculating address offset
        std::vector<uint8_t> tempData;
        int exitCode = rbcpPacketRead(busAddr, tempData, RBCP_DATA_SIZE);
        data.insert(data.end(), tempData.begin(), tempData.end());
        rbcpErrorHandler("rbcpRead", busAddr, exitCode);
    }
    if (nMod > 0) {
        int busAddr = addr + (RBCP_DATA_SIZE * i); //calculating address offset
        std::vector<uint8_t> tempData;
        int exitCode = rbcpPacketRead(busAddr, tempData, nMod);
        data.insert(data.end(), tempData.begin(), tempData.end());
        rbcpErrorHandler("rbcpRead", busAddr, exitCode);
    }
}

void BdaqRBCP::rbcpWrite(int addr, const std::vector<uint8_t>& data) {
    int nDiv = data.size() / RBCP_DATA_SIZE;
    int nMod = data.size() % RBCP_DATA_SIZE;
    int i;
    for (i = 0; i < nDiv; ++i) {
        int offset = (RBCP_DATA_SIZE * i); //calculating offset
        int busAddr = addr + offset;
        std::vector<uint8_t> tempData(RBCP_DATA_SIZE);
        tempData.assign(data.begin()+offset, data.begin()+offset+RBCP_DATA_SIZE);
        int exitCode = rbcpPacketWrite(busAddr, tempData, RBCP_DATA_SIZE);
        rbcpErrorHandler("rbcpWrite", busAddr, exitCode);
    }
    if (nMod > 0) {
        int offset = (RBCP_DATA_SIZE * i); //calculating offset
        int busAddr = addr + offset;
        std::vector<uint8_t> tempData(nMod);
        tempData.assign(data.begin()+offset, data.begin()+offset+nMod);
        int exitCode = rbcpPacketWrite(busAddr, tempData, nMod);
        rbcpErrorHandler("rbcpWrite", busAddr, exitCode);
    }
}

int BdaqRBCP::rbcpPacketRead(uint16_t addr, std::vector<uint8_t> &data, uint8_t length) {
                                unsigned char pktId=0;
    //RBCP packet header
    std::vector<uint8_t> header(RBCP_HEADER_SIZE);
    header[0] = RBCP_VER;        //ver+type
    header[1] = RBCP_CMD_RD;     //command
    header[2] = pktId;           //id
    header[3] = length;          //length in bytes
    header[4] = (addr>>24)&0xff; //address (big endian)
    header[5] = (addr>>16)&0xff; //address (big endian)
    header[6] = (addr>>8)&0xff;  //address (big endian)
    header[7] = addr&0xff;       //address (big endian)
    //Send packet and check ACK
    int r=NO_ACK;
    std::vector<uint8_t> ackPacket(RBCP_HEADER_SIZE+length);
    while (r != ACK_SUCCESS && pktId < MAX_TRIALS) {
        //Sending RBCP packet through UDP
        udp.send(boost::asio::buffer(header, RBCP_HEADER_SIZE)); //maybe check return value?
        r = rbcpCheckAck(ackPacket, pktId);
        ++pktId;
        header[2] = pktId;
    }
    //copy read data from ACK packet to data vector
    if (r == ACK_SUCCESS) data.assign(ackPacket.begin()+8, ackPacket.end());
    return r;
}

int BdaqRBCP::rbcpPacketWrite(uint16_t addr, const std::vector<uint8_t>& data,
                                uint8_t length) {
    unsigned char pktId=0;
    //RBCP packet header
    std::vector<uint8_t> header(RBCP_HEADER_SIZE);
    header[0] = RBCP_VER;        //ver+type
    header[1] = RBCP_CMD_WR;     //command
    header[2] = pktId;           //id
    header[3] = length;          //length in bytes
    header[4] = (addr>>24)&0xff; //address (big endian)
    header[5] = (addr>>16)&0xff; //address (big endian)
    header[6] = (addr>>8)&0xff;  //address (big endian)
    header[7] = addr&0xff;       //address (big endian)
    //Appending data
    std::vector<uint8_t> packet;
    packet.reserve(header.size()+length);
    packet.insert(packet.end(), header.begin(), header.end());
    packet.insert(packet.end(), data.begin(), data.end());
    //Send packet and check ACK
    int r=NO_ACK;
    std::vector<uint8_t> ackPacket(RBCP_HEADER_SIZE+length);
  
    //udp.send(boost::asio::buffer(packet, RBCP_HEADER_SIZE+length)); //maybe check return value?
    //return ACK_SUCCESS;

    while (r != ACK_SUCCESS && pktId < MAX_TRIALS) {
        //Sending RBCP packet through UDP
        udp.send(boost::asio::buffer(packet, RBCP_HEADER_SIZE+length)); //maybe check return value?
        if (BYPASS_WRITE_ACK) return ACK_SUCCESS;
        r = rbcpCheckAck(ackPacket, pktId);
        ++pktId;
        packet[2] = pktId;
    }
    return r;
}

int BdaqRBCP::rbcpCheckAck(std::vector<uint8_t>& ackPacket, unsigned char pktId) {
    //Listening for ACKnowledgement packet
    unsigned char waitCounter=0;
    while (udp.available() == 0 && waitCounter < ACK_WAIT_COUNTER){
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        ++waitCounter;
    }
    //No ACK packet received
    if (udp.available()==0) return NO_ACK;
    //Getting ACK packet
    size_t receivedLength = udp.receive(boost::asio::buffer(ackPacket));
    //Testing ACK packet size
    if (receivedLength < RBCP_HEADER_SIZE) return ACK_SHORT;
    //Checking ACK packet ID
    if (ackPacket[2] != pktId) return ACK_ID_MISMATCH;
    //Checking for bus error: returns the byte count that was executed normally
    if ((ackPacket[1] & 0x0F) != 0x8) return ackPacket[3]; //ranges from 0 to 0xFF
    //Guess we're good now...
    return ACK_SUCCESS;
}

void BdaqRBCP::rbcpErrorHandler(std::string id, uint16_t addr, int code) {
    std::stringstream stream;
    stream << "0x";
    stream << std::hex << std::setfill('0') << std::setw(sizeof(uint16_t)*2) << std::uppercase;
    stream << addr;
    std::string a(stream.str());
    switch(code){
        case ACK_SUCCESS     : return; break;
        case NO_ACK          : throw std::runtime_error("BdaqRBCP::" + id + ": NO_ACK at address: " + a + ". Is Ethernet really up?"); break;
        case ACK_SHORT       : throw std::runtime_error("BdaqRBCP::" + id + ": ACK_SHORT at address: " + a + "."); break;
        case ACK_ID_MISMATCH : throw std::runtime_error("BdaqRBCP::" + id + ": ACK_ID_MISMATCH at address: " + a + "."); break;
        case 0 ... 255       : throw std::runtime_error("BdaqRBCP::" + id + ": ACK_BUS_ERROR at address: " + a + ". Number of sucessfully executed bytes: " + std::to_string(code) + "."); break;
        default              : throw std::runtime_error("BdaqRBCP::" + id + ": ACK_UNKNOWN_ERROR at address: " + a + "."); break;
    }
}
