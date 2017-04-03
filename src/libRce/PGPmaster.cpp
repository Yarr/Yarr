#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include "PGPmaster.hh"

#include <AxiStreamDma.h>

#include <chrono>
namespace Rce{

PGPmaster* PGPmaster::_instance=0;
std::mutex PGPmaster::_guard;


// RX Thread
void * PGPmaster::rxRun ( void *t ) {
   PGPmaster *ti;
   ti = (PGPmaster *)t;
   ti->rxHandler();
   pthread_exit(NULL);
   return(NULL);
}


//! Stop link

// Open link and start threads
void PGPmaster::openLink () {
   std::stringstream err;
   std::stringstream tmp;
   err.str("");
   //  closeLink();
   runEnable_=true;

   const char path[]="/dev/axi_stream_dma_0";
   _fd = ::open(path,O_RDWR | O_NONBLOCK);

   if ( _fd < 0 ) {
      tmp.str("");
      tmp << "MultDestPgp::open -> Could Not Open AXIS path " << path <<std::endl;
      throw tmp.str();
   }
   //reset the HSIO trigger and receiver in case there was a crash
   

   // Start rx thread
   if ( pthread_create(&rxThread_,NULL,rxRun,this) ) {
      err << "CommLink::open -> Failed to create rxThread"  <<std::endl;
      std::cout << err.str();
      closeLink();
      throw(err.str());
   }
#ifdef ARM
   else pthread_setname_np(rxThread_,"cLinkRxThread");
#endif

   usleep(1000); // Let threads catch up
   sendCommand("STOP_RUN"); //stop run
   sendCommand("SOFT_RESET"); //rst receiver

}

// Stop threads and close link
void PGPmaster::closeLink () {

   // Stop the thread
   runEnable_ = false;

   // Wake up threads
   //dataThreadWakeup();
   usleep(1100); // Give enough time to threads to stop

   // Wait for thread to stop
   pthread_join(rxThread_, NULL);
   if(_fd>0) ::close(_fd);
   _fd       = -1;
}

// Receive Thread
void PGPmaster::rxHandler() {
   fd_set             fds;
   int32_t            maxFd;
   struct timeval     timeout;

   // While enabled
   while ( runEnable_ ) {

      FD_ZERO(&fds);
      FD_SET(_fd,&fds);
      maxFd=_fd;
      // Setup timeout
      timeout.tv_sec  = 0;
      timeout.tv_usec = 1000;

      // Select
      if ( maxFd < 0 ) usleep(1000); // Nothing to listen to
      else if ( select(maxFd+1, &fds, NULL, NULL, &timeout) <= 0 ) continue;

      // Process each dest
      if ( FD_ISSET(_fd, &fds) ) {
	// Receive
	receive ();
      }
   }
}


// Set max rx size
void PGPmaster::setMaxRxTx (uint32_t size) {
  std::stringstream err;

   if ( runEnable_ ) {
     err << "CommLink::setMaxRxTx -> Cannot set maxRxTx while open" << std::endl;
     std::cout << err.str();
     throw(err.str());
   }

   maxRxTx_ = size;

}

// Wake in main thread
void PGPmaster::mainThreadWait(uint32_t usec) {
   struct timespec timeout;

   pthread_mutex_lock(&mainMutex_);

   clock_gettime(CLOCK_REALTIME,&timeout);

   timeout.tv_sec  += usec / 1000000;
   timeout.tv_nsec += (usec % 1000000) * 1000;

   pthread_cond_timedwait(&mainCondition_, &mainMutex_, &timeout);
   pthread_mutex_unlock(&mainMutex_);
}

// Wakeup main thread
void PGPmaster::mainThreadWakeup() {
   pthread_cond_signal(&mainCondition_);
}



PGPmaster* PGPmaster::instance(){
  if( ! _instance){
    std::unique_lock<std::mutex> ml(_guard);
    if( ! _instance){
      _instance=new PGPmaster;
    }
  }
  return _instance;
}
    

PGPmaster::PGPmaster():_receiver(0), _tid(0),
		       _status(0), _data(0), _current(0), _handshake(false),_blockread(false),_fd(-1){
// Constructor
   pthread_mutex_init(&mainMutex_,NULL);

   pthread_cond_init(&mainCondition_,NULL);

  _rxData=new uint32_t* [16];
  for(int i=0;i<16;i++){
    _rxData[i]=new uint32_t [8192];
    _size[i]=0;
  }
  openLink();
}
PGPmaster::~PGPmaster(){
  for(int i=0;i<16;i++)delete [] _rxData[i];
  delete [] _rxData;
  closeLink();
}

  void PGPmaster::setReceiver(Receiver* receiver){
  _receiver=receiver;}
Receiver* PGPmaster::receiver(){return _receiver;}

 const uint32_t PGPmaster::writeRegister(const std::string name,uint32_t const value) {    
    const uint32_t address=_reg[name];
    return writeRegister(address,value);
  }
   const uint32_t PGPmaster::readRegister(const std::string name, uint32_t &value) {    
    const uint32_t address=_reg[name];
    return readRegister(address,value);
  }
 const uint32_t PGPmaster::sendCommand(const std::string name) {
   const uint32_t opcode=_opcode[name];
   return sendCommand(opcode);
 }
  const std::string PGPmaster::getName(const uint32_t address) {
    for(auto r: _reg) if(r.second==address) return r.first; 
    return "UNKNOWN";
  }

  uint32_t PGPmaster::readRegister(uint32_t address, uint32_t &value){
    return rwRegister(false, address, 0, value);
  }
  uint32_t PGPmaster::writeRegister(uint32_t address, uint32_t value){
    uint32_t dummy;
    return rwRegister(true, address, value, dummy);
  }
  
  uint32_t PGPmaster::rwRegister(bool write, uint32_t address, uint32_t value,uint32_t &retvalue){
    _tid++;
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    uint32_t   txSize;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=1; //VC
    _txData[0]=_tid;
    _txData[1]= write?0x40000000 : 0;
    _txData[1]|= (address&0x3fffffff); // write address
    _txData[2]=value;
    _txData[3]=0;
    txSize=16;
    std::unique_lock<std::mutex> pl( _data_mutex );
    int retval=axisWrite(_fd, _txData, txSize,firstUser,lastUser,axisDest);
    if(retval<=0){
      printf("Could not write to PGP device.\n");
      return RECEIVEFAILED;
    }
    // boost::posix_time::time_duration timeout= boost::posix_time::microseconds(RECEIVETIMEOUT/1000);

    std::cv_status signalled=_data_cond.wait_for(pl,std::chrono::microseconds(10*RECEIVETIMEOUT/1000));
    if(signalled==std::cv_status::timeout){ //timeout. Something went wrong with the data.
      printf("PGP Write Register: No reply from front end.\n");
      return RECEIVEFAILED;
    }
    retvalue=_data;
    return _status;
  }
  
  uint32_t PGPmaster::blockWrite(uint32_t* data, int size, bool handshake,bool byteswap){
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    uint32_t   txSize;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=3; //VC
    _txData[0]=0;
    _txData[1]= handshake? 1 : 0;
    _txData[2]=0;
    _txData[3]=0;
    txSize=16+size*sizeof(uint32_t);
    uint32_t* payload=&_txData[4];
   for(int i=0;i<size;i++){
#ifdef SWAP_DATA
#warning Swapping of data turned on
     if(byteswap)
       payload[i]= ((data[i]&0xff)<<8) | ((data[i]&0xff00)>>8) |
	 ((data[i]&0xff0000)<<8) | ((data[i]&0xff000000)>>8);
     else
       payload[i]=data[i]<<16 | data[i]>>16;
#else
     if(byteswap)
       payload[i]= ((data[i]&0xff)<<24) | ((data[i]&0xff00)<<8) |
	 ((data[i]&0xff0000)>>8) | ((data[i]&0xff000000)>>24);
     else{
       payload[i]=data[i];
     }
#endif
   }
   _handshake=handshake;
   if(handshake){
     _data_mutex.lock();
   }
   int retval=axisWrite(_fd, _txData, txSize,firstUser,lastUser,axisDest);
   if(retval<=0){
     printf("Could not write to PGP device.\n");
     return RECEIVEFAILED;
   }
   if(handshake){
     std::unique_lock<std::mutex> pl(_data_mutex, std::adopt_lock);
     std::cv_status signalled=_data_cond.wait_for(pl,std::chrono::microseconds(RECEIVETIMEOUT/1000));
     if(signalled==std::cv_status::timeout){ //timeout. Something went wrong with the data.
       printf("PGP Write Register: No reply from front end.\n");
       return RECEIVEFAILED;
     }
   } 
   return 0;
  }

  uint32_t PGPmaster::blockRead(uint32_t* data, int size, std::vector<uint32_t>& retvec){
    _blockread=true;
    blockWrite(data,size,true,false);
    _blockread=false;
    usleep(100);
    while(nBuffers()!=0){
      unsigned char *header, *payload;
      uint32_t headerlen, payloadlen;
      currentBuffer(header, headerlen, payload, payloadlen);
      if(header[2]==0x1f){
	discardCurrentBuffer();
	continue; //handshake
      }
      payloadlen/=sizeof(uint32_t);
      uint32_t* ptr=(uint32_t*)payload;
      for(uint32_t i=0;i<payloadlen;i++) retvec.push_back(*ptr++);
      discardCurrentBuffer();
      break;
    }
    return 0;
  }

  uint32_t PGPmaster::readBuffers(std::vector<unsigned char>& retvec){
    unsigned char *header, *payload;
    uint32_t headerlen, payloadlen;
    uint32_t count=0;
    while(nBuffers()!=0){
      count++;
      currentBuffer(header, headerlen, payload, payloadlen);
      for(uint32_t i=0;i<payloadlen;i++) retvec.push_back(payload[i]);
      if(payloadlen%3!=0){
	retvec.push_back(0);
       if(payloadlen%3==1) retvec.push_back(0);
      }
      discardCurrentBuffer();
    }
    return count;
  }
 
  uint32_t PGPmaster::sendCommand(unsigned char opcode, uint32_t context){
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    uint32_t   txSize;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=0; //VC
    _txData[0]=context&0xffffff;
    _txData[1]= (uint32_t)opcode;
    _txData[2]=0;
    _txData[3]=0;
    txSize=16;
    int retval=axisWrite(_fd, _txData, txSize,firstUser,lastUser,axisDest);
    if(retval<=0){
      printf("Could not write to PGP device.\n");
      return RECEIVEFAILED;
    }
    return 0;
  }

  uint32_t PGPmaster::sendFragment(uint32_t *data, uint32_t size){
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=2; //VC
    int retval=axisWrite(_fd, data, size*sizeof(uint32_t),firstUser,lastUser,axisDest);
    if(retval<=0){
      printf("Could not write to PGP device.\n");
      return RECEIVEFAILED;
    }
    return 0;
  }

  int count=0;
  void PGPmaster::receive(){
    const uint32_t headerSize=8;
    int32_t  ret;
    uint32_t firstUser;
    uint32_t lastUser;
    uint32_t axisDest;
    ret = axisRead(_fd, _rxData[_current], 4096,&firstUser,&lastUser,&axisDest);
    if(ret == 0){
      std::cout<<"Receive: no data"<<std::endl;
      return;
    }
    // Bad size or error
    if ( (ret < 0) || (ret % 4) != 0 || (ret-4) < 5 || lastUser ) {
      std::cout << "MultDestAxis::receive -> "
		<< "Error in data receive. Rx=" << std::dec << ret
		<< ", Dest=" << std::dec << axisDest 
		<< ", Last=" << std::dec << lastUser << std::endl;
      unsigned char* rxd=(unsigned char*)_rxData[_current];
            for(int i=0;i<ret;i++)std::cout<<std::hex<<(unsigned)rxd[i]<<" "<<std::dec;
      std::cout<<std::endl;
      return;
    }
    _size[_current]=ret/sizeof(uint32_t);
    if (axisDest==1){ // Register
      uint32_t tid=_rxData[_current][0];
           std::cout<<"Received Register"<<std::endl;
      if(tid!=_tid){
	printf ("Bad tid\n");
      }
      _status=_rxData[_current][3];
      _data=_rxData[_current][2];
      std::unique_lock<std::mutex> pl( _data_mutex );
      _data_cond.notify_one();
    } else if (axisDest==0){ // data
      if(_receiver!=NULL && _blockread==false){
	PgpData pgpdata;
	pgpdata.header=(unsigned char*)_rxData[_current];
	pgpdata.payload=&_rxData[_current][8];
	pgpdata.payloadSize=ret/sizeof(uint32_t) - headerSize;
        _receiver->receive(&pgpdata);
      }else{
	std::vector<uint32_t> rxdata;
	for(uint32_t i=0;i<ret/sizeof(uint32_t);i++)rxdata.push_back(_rxData[_current][i]);
	_buffers.push_back(rxdata);
      }
      if(_handshake){
	_handshake=false;
	std::unique_lock<std::mutex> pl( _data_mutex );
	_data_cond.notify_one();
      }
    } else if (axisDest==2){ // Atlas Event Fragment

      if(_receiver!=0){
	PgpData pgpdata;
	unsigned char header[32];
	header[2]=30; //TDCREADOUT
	pgpdata.header=header;
	pgpdata.payload=&_rxData[_current][0];
	pgpdata.payloadSize=ret/sizeof(uint32_t);
	_receiver->receive(&pgpdata);
      }
    }else{
      std::unique_lock<std::mutex> pl( _data_mutex );
      _data_cond.notify_one();
    }
    _current==15 ? _current=0: _current=_current+1;
  }
  
  uint32_t PGPmaster::nBuffers(){
    return _buffers.size();
  }

  int PGPmaster::currentBuffer(unsigned char*& header, uint32_t &headerSize, unsigned char*&payload, uint32_t &payloadSize){
    int retval=1;
    if(_buffers.empty()){
      header=0;
      headerSize=0;
      payload=0;
      payloadSize=0;
      retval=1;
    }else{
      std::vector<uint32_t> &data=*_buffers.begin();
      header=(unsigned char*)&data[0];
      headerSize=8*sizeof(uint32_t);
      payload=(unsigned char*)&data[8];
      payloadSize =(data.size()-8)*sizeof(uint32_t);
      retval=0;
    }
    return retval;
  }
   
  int PGPmaster::discardCurrentBuffer(){
    int retval=1;
    if(_buffers.empty()){
      retval=1;
    }else{
      _buffers.pop_front();
      retval=0;
    }
    return retval;
  }
  void PGPmaster::getOldData(int i, uint32_t *&data, int &size){
    int index=_current-i;
    if(index<0)index+=16;
    data=_rxData[index];
    size=_size[index];
  }

 
std::map<const std::string,const uint32_t> PGPmaster::_reg= { // needs some work
  {"CHANNEL_IN_MASK",0x0},
  {"CALIB_MODE",0x3},
  {"RESET_INPUT_DELAYS",0x7},
  {"CLOCK_SELECT",0xa},
  {"TRIGGER_MASK",0xb},
  {"CHANNEL_OUT_MASK",0xd},  
  {"L1A_ROUTE",0x1b}
} ;

std::map<const std::string,const uint32_t> PGPmaster::_opcode=
  {
    {"START_RUN",0x03},
    {"PAUSE_RUN",0x04},
    {"STOP_RUN",0x05},
    {"RESUME_RUN",0x06},
    {"RESUME_SET_MARKER",0x07},
    {"REBOOT",0x08},
    {"SOFT_RESET",0x09},
    {"WRITE_PHASE_CALIB",0x10},
    {"SET_CORE_ACTIVE",0x11},
    {"SET_CORE_INACTIVE",0x12}
  };

}
