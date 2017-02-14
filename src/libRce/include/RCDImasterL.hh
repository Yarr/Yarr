// 
// Implements the master RCDI interface
// 
// Martin Kocian, SLAC, 6/6/2009
//

#ifndef RCDIMASTER_H
#define RCDIMASTER_H

#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdint.h>
namespace PgpTrans {

  class Receiver;

  class RCDImaster{ 
  public:
    enum ERRCODES{SENDFAILED=666, RECEIVEFAILED=667};
    enum TIMEOUTS{RECEIVETIMEOUT=100000000}; //in ns
    static RCDImaster* instance();
    void setFd(int fd){_fd=fd;}
    virtual void receive();
    uint32_t rwRegister(bool write, uint32_t address, uint32_t value, uint32_t &retvalue);
    uint32_t writeRegister(uint32_t address, uint32_t value);
    uint32_t readRegister(uint32_t address, uint32_t& value);
    uint32_t blockWrite(uint32_t* data, int size, bool handshake, bool byteswap);
    uint32_t blockRead(uint32_t* data, int size, std::vector<uint32_t>& retvec);
    uint32_t readBuffers(std::vector<unsigned char>& retvec);
    uint32_t sendCommand(unsigned char opcode, uint32_t context=0);
    uint32_t sendFragment(uint32_t *data, uint32_t size);
    void setReceiver(Receiver* receiver);
    Receiver* receiver();
    uint32_t nBuffers();
    int currentBuffer(unsigned char*& header, uint32_t &headerSize, unsigned char*&payload, uint32_t &payloadSize);
    int discardCurrentBuffer();
    void getOldData(int i, uint32_t *&data, int &size);
  protected:
    RCDImaster();
    ~RCDImaster();
  private:
    Receiver* _receiver;
    uint32_t _tid;
    //    boost::mutex _data_mutex;
    //    boost::condition_variable _data_cond;
    std::mutex _data_mutex;
    std::condition_variable  _data_cond;
    uint32_t _status;
    uint32_t _data;
    std::list<std::vector<uint32_t> > _buffers;
    uint32_t _txData[4096];
    uint32_t **_rxData;
    int _size[16];
    int _current;
    bool _handshake;
    bool _blockread;
    //int m_counter;
    int _fd;
    //    static boost::mutex _guard;
    static std::mutex _guard;
    static RCDImaster* _instance;
  };
    
}//namespace

#endif
