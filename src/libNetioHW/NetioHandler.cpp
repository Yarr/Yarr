#include "NetioHandler.h"
#include <memory>

#include "logging.h"

namespace {
  auto nlog = logging::make_log("Netio::Handler");
}

// used for flush buffer
bool doFlushBuffer = false;

//MW: FIX CLANG COMPILATION
// TODO default constructor?
NetioHandler::NetioHandler(std::string contextStr, std::string felixHost,
               uint16_t felixTXPort, uint16_t felixRXPort,
               size_t queueSize) :
    m_felixHost(felixHost), m_felixTXPort(felixTXPort), m_felixRXPort(felixRXPort),
    m_queueSize(queueSize)
{
  nlog->debug("### NetioHandler::NetioHandler() -> Setting up context");
  m_activeChannels=0;
  m_context = new netio::context(contextStr);
  m_netio_bg_thread = std::thread( [&](){m_context->event_loop()->run_forever();} );
  handlerDataCount = 0;
}

NetioHandler::~NetioHandler() {
  nlog->debug("### NetioHandler::~NetioHandler()");
  nlog->debug("###  Stopping communication with FELIX:");
  nlog->debug("###   -> Closing send sockets...");

  nlog->debug("###   -> Clearing sub sockets...");
  m_sub_sockets.clear();
  nlog->debug("###   -> Stopping event loop...");
  m_context->event_loop()->stop();
  nlog->debug("###   -> Background thread joining...");
  m_netio_bg_thread.join();
  nlog->debug("###  Cleaning up buffers and utilities:");
  nlog->debug("###   -> Clearing monitors...");
  m_monitors.clear();

  nlog->debug("###  Summary of NETIO Message errors (netio msg too small):");
  for(auto it = m_msgErrors.cbegin(); it != m_msgErrors.cend(); ++it) {
    nlog->debug(" CHN:{} SUM:{}",
                it->first, it->second);
  }

  nlog->debug("###   -> Clearing queues...");
  m_pcqs.clear();
  nlog->debug("### NetioHandler::~NetioHandler() -> Clean shutdown.");
}

void NetioHandler::startChecking(){
  for (uint32_t i=0; i<m_monitors.size(); ++i) {
    m_monitors[i].startMonitor();
  }
}

void NetioHandler::stopChecking(){
  nlog->warn("### NetioHandler -> DON'T CALL stopChecking() for monitors!");
}

bool NetioHandler::isStable(size_t monitorID) {
  const std::map<uint64_t, bool>& stab = m_monitors[monitorID].getStability();
  for (auto it=stab.begin(); it!=stab.end(); ++it){
    if (!it->second) return false;
  }
  return true;
}

bool NetioHandler::isAllStable() {
  for (uint32_t i=0; i<m_monitors.size(); ++i){
    if ( !isStable(i) ) return false;
  }
  return true;
}

void NetioHandler::setFlushBuffer(bool status){
	doFlushBuffer = status;
}

int NetioHandler::getDataCount() const {
        return handlerDataCount;
}

//-----------------------------------------------------------------------------------------
void NetioHandler::addChannel(uint64_t chn){

  m_channels.push_back(chn);
  nlog->info("### NetioHandler -> Adding channel: {}");
  m_pcqs[chn] = std::make_shared<FollyQueue>(m_queueSize);

  m_msgErrors[chn] = 0;

  try {
    //AddChannel gets a callback function ptr, and bind it to the socket.
    m_sub_sockets[chn] =  new netio::low_latency_subscribe_socket( m_context, [&](netio::endpoint& ep, netio::message& msg) 
      {
	//static int event_number = 0;
        //uint32_t cid = chn;
	const uint32_t my_headersize = sizeof(felix::base::FromFELIXHeader);
	size_t msg_size = msg.size();
        std::vector<uint8_t> data = msg.data_copy();

	if (doFlushBuffer)
		return;

        uint32_t offset = my_headersize; // useful words start from end of header.

        if(msg_size > my_headersize){ //Extract all the channels from message

          // The first 8 bytes is felix header.
          felix::base::FromFELIXHeader header; // Parse header
          memcpy(&header, (const void*)&data[0], my_headersize);// would memmove be better here?

	  //For testing
	  //if(msg_size !=  header.length)
	  //	printf("\n WARNING: header length: %d instead of %d. \n", header.length, msg_size);
	  
     	  //if(m_verbose) std::cout << "NetIO msg elinkid:" << header.elinkid << ", gbtid:" << header.gbtid << std::endl;
          //uint32_t chn=(header.elinkid>>1);

          // RS: Remove header parse
          uint32_t my_chn=header.elinkid;
	  my_chn += header.gbtid*64; 

          // Copy a whole number of 32-bit words
          uint32_t numWords = (uint32_t)((msg_size-offset + 3)/4);

          // Strip packets are byte-wise, so no check on length here

          //each RD53A word is 4 bytes (32 bits)
          if((offset+4*numWords) != msg_size && m_feType == "rd53a") {
                nlog->warn("WARNING: the message size is not compatible with RD53A data format.");
          }

	  if(numWords==0)
		return;

	  uint32_t *buffer = new uint32_t[numWords]; 
          // Copy number of bytes to avoid dereferencing a bad word on the end
          memcpy(buffer, &data[offset], msg_size-offset);

	  //Sasha: print out the data
	  nlog->debug("msg size: {}", numWords);
          //for(uint32_t i=0; i<numRd53AWords; i++ ) { //&&  i < 13; ++i) {
	  //      printf("event number: %i ", event_number);
          //	printf("%08x ", buffer[i]); 
	  //	printf("l1id[%i] tag[%i] bcid[%i]",  (0x1F & (buffer[1] >> 20)), (0x1F & (buffer[1] >> 15)), (0x7FFF & buffer[1]));
          //}
	  //printf(" .... \n");
	  //event_number++;
          
	  RawData* rd = new RawData(my_chn, buffer, numWords);
	  std::unique_ptr<RawData> rdp =  std::unique_ptr<RawData>(rd);
	  rawData.pushData(std::move(rdp));

          ++handlerDataCount;
        } else  { 
        	nlog->warn("WARNING: NetIO message is shorter than {} bytes. It is {} bytes.", my_headersize, data.size());
          	//m_msgErrors[cid]++;
		return;
        }

	return;
    });

    m_sub_sockets[chn]->subscribe(chn, netio::endpoint(m_felixHost, m_felixRXPort));
    //std::cout << "Should be subscribed to " << m_felixHost << ":" << m_felixRXPort << std::endl;
  } catch(...) {
    nlog->error("### NetioHandler::addChannel({}) -> ERROR. Failed to activate channel.", chn);
    return;
  }
  nlog->debug("### NetioHandler::addChannel({}) -> Success. Queue and socket-pair created, subscribed.", chn);
  m_activeChannels++;

  //std::this_thread::sleep_for(std::chrono::milliseconds(10));

}


//---------------------------------------------------------------------------
void NetioHandler::delChannel(uint64_t chn){
  nlog->debug("### NetioHandler::delChannel({})", chn);
  std::vector<uint64_t>::iterator it;
  it=std::find (m_channels.begin(),m_channels.end(),chn);
  if(it!=m_channels.end()){
    nlog->debug("### NetioHandler::delChannel({}) -> unsubscribe", chn);
    m_channels.erase(it);
    //SHIT: please do not unsubscribe: because felixcore/netio doesn't like it
    m_sub_sockets[chn]->unsubscribe(chn, netio::endpoint(m_felixHost, m_felixRXPort));
    delete m_sub_sockets[chn];
    m_sub_sockets.erase(chn);
  }
}
