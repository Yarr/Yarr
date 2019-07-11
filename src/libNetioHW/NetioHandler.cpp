#include "NetioHandler.h"
#include <memory>

#include "NetioFei4Records.h"
// used for flush buffer
bool doFlushBuffer = false;

//MW: FIX CLANG COMPILATION
// TODO default constructor?
NetioHandler::NetioHandler(std::string contextStr, std::string felixHost,
               uint16_t felixTXPort, uint16_t felixRXPort,
               size_t queueSize, bool verbose) :
    m_felixHost(felixHost), m_felixTXPort(felixTXPort), m_felixRXPort(felixRXPort),
    m_queueSize(queueSize), m_verbose(verbose)
{
  if (m_verbose) { std::cout << "### NetioHandler::NetioHandler() -> Setting up context. \n"; }
  m_activeChannels=0;
  m_context = new netio::context(contextStr);
  m_netio_bg_thread = std::thread( [&](){m_context->event_loop()->run_forever();} );
  handlerDataCount = 0;
}

NetioHandler::~NetioHandler() {
  if (m_verbose) { std::cout << "### NetioHandler::~NetioHandler()\n"; }
  if (m_verbose) { std::cout << "###  Stopping communication with FELIX:\n"
                             << "###   -> Closing send sockets...\n"; }
  for (auto socketIt : m_send_sockets ) {
    if (socketIt.second->is_open()) socketIt.second->disconnect();
  }
  if (m_verbose) { std::cout << "###   -> Clearing send and sub sockets...\n"; }
  m_send_sockets.clear();
  m_sub_sockets.clear();
  if (m_verbose) { std::cout << "###   -> Stopping event loop...\n"; }
  m_context->event_loop()->stop();
  if (m_verbose) { std::cout << "###   -> Background thread joining...\n"; }
  m_netio_bg_thread.join();
  if (m_verbose) { std::cout << "###  Cleaning up buffers and utilities:\n"
                             << "###   -> Clearing monitors...\n"; }
  m_monitors.clear();

  if (m_verbose) { std::cout << "###  Summary of NETIO Message errors (netio msg too small):\n";
    for(auto it = m_msgErrors.cbegin(); it != m_msgErrors.cend(); ++it)
    {
      std::cout << it->first << " CHN:" << it->first << " SUM:" << it->second << "\n";
    }
  }

  if (m_verbose) { std::cout << "###   -> Clearing queues...\n"; }
  m_pcqs.clear();
  if (m_verbose) { std::cout << "### NetioHandler::~NetioHandler() -> Clean shutdown. \n"; }
}

void NetioHandler::monitorSetup(size_t sensitivity, size_t delay, size_t numOf){
  m_sensitivity=sensitivity;
  m_delay=delay;
  if (numOf==0) {
    for (uint32_t i=0; i<m_activeChannels; ++i){
      m_monitors.push_back( QueueMonitor(i, std::ref(m_monitor_config_basic[i]), m_pcqs, m_sensitivity, m_delay) );
    }
  } else {
    for (uint32_t i=0; i<numOf; ++i){
      m_monitors.push_back( QueueMonitor(i, std::ref(m_monitor_config[i]), m_pcqs, m_sensitivity, m_delay) );
    }
  }
}

void NetioHandler::configureMonitors(size_t sensitivity, size_t delay) {
  m_sensitivity=sensitivity;
  m_delay=delay;
  if (m_verbose) {
    std::cout << "### NetioHandler::configureMonitors(sensitivity=" << sensitivity << ", delay=" << delay <<")\n"
              << "###   -> Making monitor mapping for active channels: " << m_activeChannels << "\n";
  }
  switch(m_monitor_mode)
  {
    case single:
      {
        std::cout << "###  -> 1 Monitor per single elink mode.\n";
        for (uint32_t i=0; i<m_activeChannels; ++i) {
          m_monitor_config_dynamic.insert( std::pair<uint32_t, std::vector<uint64_t>>(i, {m_channels[i]}) );
          m_monitors.emplace_back( QueueMonitor(i, std::ref(m_monitor_config_dynamic[i]), m_pcqs, m_sensitivity, m_delay) );
        }
      }
      break;
    case dual:
      {
        //if (m_activeChannels/2 != 0) { std::cout << "### ERROR -> Active channels number are not the "}
        std::cout << "###  -> 1 Monitor per 2 elink mode.\n";
        for (uint32_t i=0; i<m_activeChannels; i=i+2) {
          m_monitor_config_dynamic.insert( std::pair<uint32_t, std::vector<uint64_t>>(i/2, {m_channels[i], m_channels[i+1]}) );
          m_monitors.emplace_back( QueueMonitor(i/2, std::ref(m_monitor_config_dynamic[i/2]), m_pcqs, m_sensitivity, m_delay) );
        }
      }
      break;
    case quad:
      {
        //if (m_activeChannels/2 != 0) { std::cout << "### ERROR -> Active channels number are not the "}
        std::cout << "###  -> 1 Monitor per 4 elink mode.\n";
        for (uint32_t i=0; i<m_activeChannels; i=i+4) {
          m_monitor_config_dynamic.insert( std::pair<uint32_t, std::vector<uint64_t>>(i/4, {m_channels[i], m_channels[i+1], m_channels[i+2], m_channels[i+3]}) );
          m_monitors.emplace_back( QueueMonitor(i/4, std::ref(m_monitor_config_dynamic[i/4]), m_pcqs, m_sensitivity, m_delay) );
        }
      }
      break;
  }
}

void NetioHandler::startChecking(){
  for (uint32_t i=0; i<m_monitors.size(); ++i) {
    m_monitors[i].startMonitor();
  }
}

void NetioHandler::stopChecking(){
  std::cout << "### NetioHandler -> DON'T CALL stopChecking() for monitors!\n";
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

std::vector<uint32_t> NetioHandler::pushOut(uint64_t chn) {
  std::vector<uint32_t> dataVec;
  while ( !m_pcqs[chn]->isEmpty() ) {
    uint32_t record;
    m_pcqs[chn]->read(std::ref(record));
    dataVec.push_back(record);
  }
  return dataVec;
}

void NetioHandler::setFlushBuffer(bool status){
	doFlushBuffer = status;
}

int NetioHandler::getDataCount() {
        return handlerDataCount;
}

//-----------------------------------------------------------------------------------------
void NetioHandler::addChannel(uint64_t chn){

  m_channels.push_back(chn);
  std::cout << "### NetioHandler -> Adding channel: " << chn << '\n';
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

          uint32_t numWords = (uint32_t)((msg_size-offset)/4); //each RD53A word is 4 bytes (32 bits)

	  
	  if((offset+4*numWords) != msg_size && m_feType == "rd53a") // this is rd53a specific; there may need to be a similar thing for strips
		std::cout<<"\nWARNING: the message size is not compatible with RD53A data format.\n";

	  if(numWords==0)
		return;

	  uint32_t *buffer = new uint32_t[numWords]; 
	  memcpy(buffer, (uint32_t *)&data[offset], numWords*4);

	  //Sasha: print out the data
	  printf("msg size: %d \n", numWords);
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
        	std::cerr << "WARNING: NetIO message is shorter than "<<my_headersize<<" bytes. It is  " << data.size() << " bytes."<< std::endl;
          	//m_msgErrors[cid]++;
		return;
        }

	return;
    });

    m_sub_sockets[chn]->subscribe(chn, netio::endpoint(m_felixHost, m_felixRXPort));
    //std::cout << "Should be subscribed to " << m_felixHost << ":" << m_felixRXPort << std::endl;
  } catch(...) {
    std::cerr << "### NetioHandler::addChannel(" << chn << ") -> ERROR. Failed to activate channel.\n";
    return;
  }
  if (m_verbose) { std::cout << "### NetioHandler::addChannel(" << chn
                             << ") -> Success. Queue and socket-pair created, subscribed. \n"; }
  m_activeChannels++;

  //std::this_thread::sleep_for(std::chrono::milliseconds(10));

}


//---------------------------------------------------------------------------
void NetioHandler::delChannel(uint64_t chn){
    if (m_verbose) { std::cout << "### NetioHandler::delChannel(" << chn << ")" << std::endl; }
  std::vector<uint64_t>::iterator it;
  it=std::find (m_channels.begin(),m_channels.end(),chn);
  if(it!=m_channels.end()){
    if (m_verbose) { std::cout << "### NetioHandler::delChannel(" << chn << ")"
                               << " -> unsubscribe" << std::endl; }
    m_channels.erase(it);
    //SHIT: please do not unsubscribe: because felixcore/netio doesn't like it
    //m_sub_sockets[chn]->unsubscribe(chn, netio::endpoint(m_felixHost, m_felixRXPort));
    delete m_send_sockets[chn];
    delete m_sub_sockets[chn];
    m_send_sockets.erase(chn);
    m_sub_sockets.erase(chn);
  }
}
