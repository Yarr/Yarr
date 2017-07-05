#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "BocCom.h"

// helper functions for converting between byte array and IPBus header
void BocCom::header2bytes(IPBusHeader *header, uint8_t *buf)
{
    uint32_t tmp;
    tmp = (header->vers << 28) |
          (header->trans_id << 17) |
          (header->words << 8) |
          (header->type << 3) |
          (header->d << 2) |
          (header->res << 0);
    buf[3] = (tmp & 0xFF000000) >> 24;
    buf[2] = (tmp & 0x00FF0000) >> 16;
    buf[1] = (tmp & 0x0000FF00) >> 8;
    buf[0] = (tmp & 0x000000FF);
}

void BocCom::bytes2header(IPBusHeader *header, uint8_t* buf)
{
    uint32_t tmp = 0;
    tmp = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    header->vers = ((tmp & 0xF0000000) >> 28);
    header->trans_id = ((tmp & 0x0FFE0000) >> 17);
    header->words = ((tmp & 0x0001FF00) >> 8);
    header->type = ((tmp & 0x000000F8) >> 3);
    header->d = ((tmp & 0x00000004) >> 2);
    header->res = (tmp & 0x00000003);
}

void BocCom::print_header(IPBusHeader *header)
{
    printf("------------------------------------------------------------\n");
    printf("Version: %d\n", header->vers);
    printf("Transaction-ID: %03X\n", header->trans_id);
    printf("Words: %d\n", header->words);
    printf("Type: %02X\n", header->type);
    printf("Direction: %s\n", (header->d == 0) ? "to BocCom" : "from BOC");

    switch(header->res) {
        case 0:
            printf("Result: OK\n");
            break;

        case 1:
            printf("Result: PARTIAL\n");
            break;

        case 2:
            printf("Result: FAIL\n");
            break;

        case 3:
            printf("Result: RESERVED\n");
            break;

        default:
            printf("Result: undefined\n");
            break;
    }

    printf("------------------------------------------------------------\n");
}

// constructor
BocCom::BocCom(const std::string &host)
{
    // debug
    std::cout << "Connecting to BOC '" << host << "'..." << std::endl;

    // try resolve host
    struct addrinfo hints, *res, *ressave;
    int n;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    n = getaddrinfo(host.c_str(), "50000", &hints, &res);
    if(n < 0)
    {
        std::cerr << "Could not resolve host '" << host << "'. Error " <<
                           std::to_string(n) << ": " << gai_strerror(n) << std::endl;
        exit(-1);
    }

    // save resolver results
    ressave = res;

    // loop through resolved
    socketfd = -1;
    while(res)
    {
        socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        if(!(socketfd < 0))
        {
#ifdef LL_DEBUG
            // print IP address
            char IpAddr_str[INET6_ADDRSTRLEN];
            switch(res->ai_family)
            {
                case AF_INET:    inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, IpAddr_str, INET6_ADDRSTRLEN);
                                break;
                case AF_INET6:    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, IpAddr_str, INET6_ADDRSTRLEN);
                                break;
            }
            std::cout << "Connecting to '" << host << "' (IP address " << IpAddr_str << ")..." << std::endl;
#endif

            // try to connect
            if(connect(socketfd, res->ai_addr, res->ai_addrlen) == 0)
                break;

            close(socketfd);
            socketfd = -1;
        }

        // get next result in the list
        res = res->ai_next;
    }

    // free up addrinfo space
    freeaddrinfo(ressave);

    // check if we could connect to a host
    if(socketfd < 0)
    {
        std::cerr << "Could not connect to host '" << host << "'." << std::endl;
        exit(-1);
    }

    // reset transaction id
    transaction_id = 0;

    // debug output
    std::cout << "BOC revision: 0x" << std::hex << (int)getBocRev() << std::dec << std::endl;
    std::cout << "BMF north feature: 0x" << std::hex << getBmfFeatures(BMFN_OFFSET) << std::dec << std::endl;
    std::cout << "BMF south feature: 0x" << std::hex <<	getBmfFeatures(BMFS_OFFSET) << std::dec	<< std::endl;

    // check that BOC revision is good
    if((getBocRev() < 0x4) ||
       (getBmfFeatures(BMFN_OFFSET) != getBmfFeatures(BMFS_OFFSET)) ||
       ((getBmfFeatures(BMFN_OFFSET) & 0xB0) == 0x90))
    {
        std::cerr << "The BOC card is not compatible with the YaRR software. Sorry!" << std::endl;
        std::cerr << "You will need: " << std::endl;
        std::cerr << "    - BOC at least revision D" << std::endl;
        std::cerr << "    - YaRR firmware on both BMF and the BCF" << std::endl;
        std::cerr << "    - TX/RX broadcasting feature available" << std::endl;
        close(socketfd);
        exit(-1);
    }
}

// destructor
BocCom::~BocCom()
{
#ifdef LL_DEBUG
    std::cout << "Destroying Boc" << std::endl;
#endif

    // close socket
    close(socketfd);
}

uint8_t BocCom::readSingle(uint16_t addr)
{
    uint8_t ret;
    readBlock(addr, &ret, 1, false);
    return ret;
}

void BocCom::readBlock(uint16_t addr, uint8_t *val, size_t words, bool incrementing)
{
    int ret;
    IPBusHeader txheader;
    IPBusHeader rxheader;
    uint8_t txdata[8];
    uint8_t rxdata[256];

    // lock mutex
    socket_mutex.lock();

    // we need to read in chunks of max. 128 byte
    if(words > 128)
    {
        words = 128;
    }

    // prepare packet
    txheader.vers = 1;
    txheader.trans_id = transaction_id;
    txheader.words = words;
    txheader.type = incrementing ? 0x03 : 0x08;
    txheader.d = 0;
    txheader.res = 0;

#ifdef LL_DEBUG
    // print header
    print_header(&txheader);
#endif

    // copy header into byte-array
    header2bytes(&txheader, txdata);
    
    // copy address into bytearray
    txdata[7] = 0;
    txdata[6] = 0;
    txdata[5] = addr >> 8;
    txdata[4] = addr & 0xff;

    // send out data through socket
    ret = send(socketfd, txdata, 8, 0);
    if(ret < 0)
    {
        std::cerr << "Error " << std::to_string(ret) << " while sending request." << std::endl;
        exit(-1);
    }
    else if(ret != 8)
    {
        std::cerr << "Could only send " << std::to_string(ret) << " out of 8" <<
                           " bytes. This needs to be catched in future versions!"<< std::endl;
        exit(-1);
    }

    // receive response
    ret = recv(socketfd, rxdata, words+4, 0);
    if(ret < 0)
    {
        std::cerr << "Error " << std::to_string(ret) << " while receiving response." << std::endl;
        exit(-1);
    }

#ifdef LL_DEBUG
    std::cout << "Received " << ret << " bytes.\n";
#endif

    // copy into header
    bytes2header(&rxheader, rxdata);

#ifdef LL_DEBUG
    // print out header
    print_header(&rxheader);
#endif

    // copy data
    for(int i = 0; i < rxheader.words; i++)
    {
        val[i] = rxdata[4+i];
    }

    // increment transaction id
    transaction_id = transaction_id + 1;

    // unlock mutex
    socket_mutex.unlock();
}

void BocCom::read16(uint16_t high_addr, uint16_t low_addr, uint16_t *val, size_t words)
{
    int ret;
    IPBusHeader txheader;
    IPBusHeader rxheader;
    uint8_t txdata[8];
    uint8_t rxdata[512];

    // lock mutex
    socket_mutex.lock();

    // we need to read in chunks of max. 128 byte
    if(words > 128)
    {
        words = 128;
    }

    // prepare packet
    txheader.vers = 1;
    txheader.trans_id = transaction_id;
    txheader.words = words;
    txheader.type = 0x10;
    txheader.d = 0;
    txheader.res = 0;

#ifdef LL_DEBUG
    // print header
    print_header(&txheader);
#endif

    // copy header into byte-array
    header2bytes(&txheader, txdata);
    
    // copy address into bytearray
    txdata[7] = low_addr >> 8;;
    txdata[6] = low_addr & 0xff;;
    txdata[5] = high_addr >> 8;
    txdata[4] = high_addr & 0xff;

    // send out data through socket
    ret = send(socketfd, txdata, 8, 0);
    if(ret < 0)
    {
        std::cerr << "Error " << std::to_string(ret) << " while sending request." << std::endl;
        exit(-1);
    }
    else if(ret != 8)
    {
        std::cerr << "Could only send " << std::to_string(ret) << " out of 8" <<
                           " bytes. This needs to be catched in future versions!"<< std::endl;
        exit(-1);
    }

    // receive response
    ret = recv(socketfd, rxdata, 2*words+4, 0);
    if(ret < 0)
    {
        std::cerr << "Error " << std::to_string(ret) << " while receiving response." << std::endl;
        exit(-1);
    }

#ifdef LL_DEBUG
    std::cout << "Received " << ret << " bytes.\n";
#endif

    // copy into header
    bytes2header(&rxheader, rxdata);

#ifdef LL_DEBUG
    // print out header
    print_header(&rxheader);
#endif

    // copy data
    for(int i = 0; i < rxheader.words; i++)
    {
        val[i] = (uint16_t)rxdata[2*i+4] << 8;
        val[i] |= (uint16_t)rxdata[2*i+5];
    }

    // increment transaction id
    transaction_id = transaction_id + 1;

    // unlock mutex
    socket_mutex.unlock();
}

void BocCom::writeSingle(uint16_t addr, uint8_t value)
{
    writeBlock(addr, &value, 1, false);
}

void BocCom::writeBlock(uint16_t addr, uint8_t *val, size_t words, bool incrementing)
{
    IPBusHeader txheader;
    IPBusHeader rxheader;
    uint8_t txdata[256];
    uint8_t rxdata[4];
    int ret;

    // lock mutex
    socket_mutex.lock();

    // we need to send data in chunks of max. 128 bytes
    if(words > 128)
    {
        words = 128;
    }

    // prepare write request
    txheader.vers = 1;
    txheader.trans_id = transaction_id;
    txheader.words = words;
    txheader.type = incrementing ? 0x04 : 0x09;
    txheader.d = 0;
    txheader.res = 0;

#ifdef LL_DEBUG
    // print out header
    print_header(&txheader);
#endif

    // copy header into byte-array
    header2bytes(&txheader, txdata);

    // copy address into bytearray
    txdata[7] = 0;
    txdata[6] = 0;
    txdata[5] = addr >> 8;
    txdata[4] = addr & 0xff;

    // copy data into byte array
    for(size_t i = 0; i < words; i++) {
        txdata[i+8] = val[i];
    }

    // send out data through socket
    ret = send(socketfd, txdata, words+8, 0);
    if(ret < 0)
    {
        std::cerr << "Error " << std::to_string(ret) << " while sending request." << std::endl;
        exit(-1);
    }
    else if((size_t)ret != words+8)
    {
        // TODO: need to get things working
        std::cerr << "Could only send " << std::to_string(ret) << " out of " << std::to_string(words+8) <<
                     " bytes. This needs to be catched in future versions!" << std::endl;
        exit(-1);
    }

    // receive response
    ret = recv(socketfd, rxdata, 4, 0);
    if(ret < 0)
    {
        std::cerr << "Error " << std::to_string(ret) << " while receiving response." << std::endl;
        exit(-1);
    }

#ifdef LL_DEBUG
    std::cout << "Received " << ret << " bytes.\n";
#endif

    // copy into header
    bytes2header(&rxheader, rxdata);

#ifdef LL_DEBUG
    // print out header
    print_header(&rxheader);
#endif

    // increment transaction id
    transaction_id = transaction_id + 1;

    // unlock mutex
    socket_mutex.unlock();
}

// incrementing read/write
void BocCom::writeInc(uint16_t addr, uint8_t *val, size_t words)
{
    writeBlock(addr, val, words, true);
}

void BocCom::readInc(uint16_t addr, uint8_t *val, size_t words)
{
    readBlock(addr, val, words, true);
}

// non incrementing read/write
void BocCom::writeNonInc(uint16_t addr, uint8_t *val, size_t words)
{
    writeBlock(addr, val, words, false);
}

void BocCom::readNonInc(uint16_t addr, uint8_t *val, size_t words)
{
    readBlock(addr, val, words, false);
}

uint8_t BocCom::getBocRev()
{
    return readSingle(BCF_OFFSET + BCF_REVISION);
}

uint16_t BocCom::getBmfFeatures(uint16_t bmf)
{
    uint16_t features;

    // read all feature registers
    features = (uint16_t) readSingle(bmf + BMF_FEATURE_LOW);
    features |= (uint16_t) readSingle(bmf + BMF_FEATURE_HIGH) << 8;

    // combine the feature register
    return features;
}
