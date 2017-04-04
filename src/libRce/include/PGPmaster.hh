// 
// Implements the master RCDI interface
// 
// Martin Kocian, SLAC, 6/6/2009
//

#ifndef PGPMASTER_H
#define PGPMASTER_H

#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdint.h>
#include <map>
namespace Rce {
  struct PgpData{
    uint8_t *header;
    uint32_t *payload;
    uint32_t payloadSize;};

  class Receiver {
  public:
    Receiver()  {}
    virtual ~Receiver(){}
    virtual void receive(PgpData* pgpdata)=0;
  };


  class PGPmaster{ 
  public:
    enum ERRCODES{SENDFAILED=666, RECEIVEFAILED=667};
    enum TIMEOUTS{RECEIVETIMEOUT=100000000}; //in ns
    static PGPmaster* instance();
    void openLink ();
    virtual void receive();
    uint32_t rwRegister(bool write, uint32_t address, uint32_t value, uint32_t &retvalue);
    uint32_t writeRegister(uint32_t address, uint32_t value);
    uint32_t readRegister(uint32_t address, uint32_t& value);
    uint32_t blockWrite(uint32_t* data, int size, bool handshake, bool byteswap);
    uint32_t blockRead(uint32_t* data, int size, std::vector<uint32_t>& retvec);
    uint32_t readBuffers(std::vector<unsigned char>& retvec);
    uint32_t sendCommand(unsigned char opcode, uint32_t context=0);
    uint32_t sendFragment(uint32_t *data, uint32_t size);
    const uint32_t sendCommand(const std::string name);
    const uint32_t readRegister(const std::string name, uint32_t &value);
    const uint32_t writeRegister(const std::string name,uint32_t const value);
    const std::string getName(const uint32_t address);
    void setReceiver(Receiver* receiver);
    Receiver* receiver();
    uint32_t nBuffers();
    int currentBuffer(unsigned char*& header, uint32_t &headerSize, unsigned char*&payload, uint32_t &payloadSize);
    int discardCurrentBuffer();
    void getOldData(int i, uint32_t *&data, int &size);
    void setMaxRxTx ( uint32_t maxRxTx );
  protected:
    PGPmaster();
    ~PGPmaster();
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
    static PGPmaster* _instance;
  
    void closeLink ();
    pthread_t rxThread_;
  
    // Thread Routines
    static void *rxRun ( void *t );
    
    // Thread condition variables
    pthread_cond_t  mainCondition_;
    pthread_mutex_t mainMutex_;
    
    // Condition set and wait routines
    void mainThreadWait(uint32_t usec);
    void mainThreadWakeup();
    
    // Run enable
    bool runEnable_;
    
    // IO handling routines
    virtual void rxHandler();
    
    // Max RX/Tx size
    uint32_t maxRxTx_;

    static  std::map<const std::string,const uint32_t> _reg;
    static std::map<const std::string,const uint32_t> _opcode;
  };
    
}//namespace

#endif
