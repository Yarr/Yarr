#include "NetioHandler.h"

#include "NetioFei4Records.h"

NetioHandler::NetioHandler(std::string contextStr="posix", std::string felixHost="localhost",
               uint16_t felixTXPort=12340, uint16_t felixRXPort=12345,
               size_t queueSize=10000000, bool verbose=false) :
    m_felixHost(felixHost), m_felixTXPort(felixTXPort), m_felixRXPort(felixRXPort),
    m_queueSize(queueSize), m_verbose(verbose)
{
  if (m_verbose) { std::cout << "### NetioHandler::NetioHandler() -> Setting up context. \n"; }
  m_activeChannels=0;
  m_context = new netio::context(contextStr);
  m_netio_bg_thread = std::thread( [&](){m_context->event_loop()->run_forever();} );
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

void NetioHandler::addChannel(uint64_t chn){
  m_channels.push_back(chn);
  std::cout << "### NetioHandler -> Adding channel: " << chn << '\n';
  m_pcqs[chn] = std::make_shared<FollyQueue>(m_queueSize);
  m_msgErrors[chn] = 0;
  try {
    //m_send_sockets[chn] = new netio::low_latency_send_socket(m_context);
    //m_send_sockets[chn]->connect(netio::endpoint(m_felixHost, m_felixTXPort));
    m_sub_sockets[chn] =  new netio::low_latency_subscribe_socket(
      // FIXME: This is really ugly... AddChannel should also get a callback function ptr, and bind it to the socket.
      //netio::low_latency_subscribe_socket(
      m_context, [&](netio::endpoint& ep, netio::message& msg) {
        //std::cout << "NETIO: NEW MSG!!!\n";
        uint32_t cid = chn;
        std::vector<uint8_t> data = msg.data_copy();
        uint32_t offset=0;
        while(offset<data.size()){ //Extract all the channels from message
          if(data.size()-offset<sizeof(felix::base::FromFELIXHeader)){
            //std::cout << "NetIO msg too small:" << (data.size()-offset) << std::endl;
            m_msgErrors[cid]++;
            break;
          }
          // RS: Remove header parse
          felix::base::FromFELIXHeader header; // Parse header
          memcpy(&header, (const void*)&data[offset], sizeof(header));
          offset+=sizeof(header); // useful words start from end of header.

     //     //if(m_verbose) std::cout << "NetIO msg elinkid:" << header.elinkid << ", gbtid:" << header.gbtid << std::endl;
          //uint32_t chn=(header.elinkid>>1);

          // RS: Remove header parse
          uint32_t chn=header.elinkid;
          if(header.gbtid<=1){chn+=header.gbtid*32;}
          else{/*std::cout << "NetIO msg elinkid:" << header.elinkid << ", gbtid:" << header.gbtid << std::endl;*/ break;}

          uint32_t numFei4Words=(header.length-m_headersize)/m_datasize; // numWords is FEI4_WORD! Not header.length

          for(uint32_t i=0; i<numFei4Words; ++i){
            YARR_RECORD rec(0);
            rec.inner.payload1 = static_cast<uint8_t>(data[offset+2]); // field1[8:0]
            rec.inner.payload2 = static_cast<uint8_t>(data[offset+1]); // field2[8:0]
            rec.inner.payload3 = static_cast<uint8_t>(data[offset]);   // rest[16:0]
            rec.inner.type = false; // Do we need to change this hardcode?
            rec.inner.channel = static_cast<uint8_t>(chn);
            m_pcqs[chn]->write( std::move(rec.allfields) ); // write to channel's queue
            offset+=m_datasize; // datasize is 3, the FEI4 word size.
          }
        }
                                           });//);
    m_sub_sockets[chn]->subscribe(chn, netio::endpoint(m_felixHost, m_felixRXPort));
    std::this_thread::sleep_for(std::chrono::microseconds(10000)); // This is needed... :/
    //std::cout << "Should be subscribed to " << m_felixHost << ":" << m_felixRXPort << std::endl;
  } catch(...) {
    std::cout << "### NetioHandler::addChannel(" << chn << ") -> ERROR. Failed to activate channel.\n";
    return;
  }
  if (m_verbose) { std::cout << "### NetioHandler::addChannel(" << chn
                             << ") -> Success. Queue and socket-pair created, subscribed. \n"; }
  m_activeChannels++;
}

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
