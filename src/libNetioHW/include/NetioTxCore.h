#ifndef NETIOTXCORE_H
#define NETIOTXCORE_H

/********************************
 * NetioTxCore
 * Author: Carlos.Solans@cern.ch
 * Author: Roland.Sipos@cern.ch
 * Description: NetIO Transmitter
 * Date: June 2017
 *********************************/

#include "TxCore.h"
#include "BitStream.h"
#include "NetioHandler.h"
#include "netio/netio.hpp"
#include "felixbase/client.hpp"

#include <cstdint>
#include <queue>
#include <vector>
#include <mutex>


/**
 * @brief Implementation of the RxCore for NetIO communication.
 *        This class subscribes to felix using a low_latency_socket.
 **/
class NetioTxCore : virtual public TxCore {
public:
		
  /**
   * @brief Default constructor
   * Create NetIO context and low_latency_send_socket
   **/
  NetioTxCore();
  
  /**
   * @brief Destructor
   * Join the trigger thread. Disconnect the socket. 
   * Delete socket and context.
   **/
  ~NetioTxCore();
    
  /**
   * @brief append to fifo of all channels
   * @param hexStr command as hex string
   **/
  void writeFifo(std::string hexStr);

  /**
   * @brief append to fifo of all channels
   * @param value command as uint32_t
   **/
  void writeFifo(uint32_t value);

  /**
   * @brief append hexStr to channel fifo
   * @param channel channel (elink) of fifo
   * @param hexStr command as hex string
   **/
  void writeFifo(uint32_t channel, std::string hexStr);

  /**
   * @brief append value to channel fifo
   * @param channel channel (elink) of fifo
   * @param value command
   **/
  void writeFifo(uint32_t channel, uint32_t value);
  
  /**
   * @brief release the fifo for all enabled channels
   **/
  void releaseFifo();
  
  /**
   * @brief release the fifo for all given channel
   * @param channel channel (elink)
   **/
  void releaseFifo(uint32_t channel);

  /**
   * @brief release the fifo for all trigger channels
   **/
  void releaseFifo(bool trigChns);
  
  /**
   * @brief Get Mutex object to lock the core
   * @return reference to the mutex
   **/
  std::mutex & getMutex();
  
  /**
   * @brief enable a given channel (elink)
   * @param channel channel (elink) to enable
   **/
  void enableChannel(uint64_t channel);

  /**
   * @brief disable a given channel (elink)
   * @param channel channel (elink) to disable
   **/
  void disableChannel(uint64_t channel);

  /**
   * @brief check if the fifo of commands is empty
   * @return bool true if fifo is empty
   **/
  bool isCmdEmpty();

  /**
   * @brief enable the trigger
   * @param value for the trigger (0x0,0x1)
   **/
  void setTrigEnable(uint32_t value);
  
  /**
   * @brief get if the trigger is enabled
   * @return uint32_t get trigger enable
   **/
  uint32_t getTrigEnable();
  
  /**
   * @brief enable a trigger channel
   * @param channel channel (elink) to enable
   * @param enable bool new state for trigger channel
   **/
  void setTrigChannel(uint64_t channel, bool enable);

  /**
   * @brief check if the trigger is done
   * @return bool is trigger done
   **/
  bool isTrigDone();
  
  /**
   * @brief set the trigger config
   * @param cfg TRIG_CONF_VALUE config value
   **/
  void setTrigConfig(enum TRIG_CONF_VALUE cfg);
  
  /**
   * @brief set the trigger frequency
   * @param freq double frequency in Hz
   **/
  void setTrigFreq(double freq); 
  
  /**
   * @brief set the number of desired triggers
   * @param count number of desired triggers
   **/
  void setTrigCnt(uint32_t count);
  
  /**
   * @brief get trigger count
   * @return uint32_t count number of desired triggers
   **/
  uint32_t getTrigCnt();
  
  /**
   * @brief set the trigger time in seconds
   * @param time in seconds
   **/
  void setTrigTime(double time);
  
  /**
   * @brief get the trigger time in seconds
   * @return double time in seconds
   **/
  double getTrigTime();
  
  /**
   * @brief set Trigger Word Length
   * @param length of the trigger word starting at MSB
   **/
  void setTrigWordLength(uint32_t length);
  
  /**
   * @brief set the trigger words
   * @param words uint32_t pointer to trigger words
   * @param size number of words in the pointer
   **/
  void setTrigWord(uint32_t *words, uint32_t size); 
  
  /**
   * @brief abort the trigger sequence
   **/
  void toggleTrigAbort();
  
  /**
   * @brief set the trigger logic
   * @param mask
   **/
  void setTriggerLogicMask(uint32_t mask);
  
  /**
   * @brief set the trigger logic mode
   * @param mode TRIG_LOGIC_MODE_VALUE trigger logic mode
   **/
  void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode);
  
  /**
   * @brief reset the trigger logic
   **/
  void resetTriggerLogic();
  
  /**
   * @brief get the number of triggers in
   * @return uint32_t get number of triggers
   **/
  uint32_t getTrigInCount();
  
  /**
   * @brief Enable or disable verbose mode
   * @param enable boolean value
   **/
  void setVerbose(bool enable);
  
  /**
   * @brief read configuration from json
   * @param j reference to json
   **/
  void fromFileJson(json& j);
  
  /**
   * @brief read configuration from string
   * @param s string configuration to decode
   **/
  void fromString(std::string s);
  
  /**
   * @brief write configuration to json
   * @param j reference to json
   **/
  void toFileJson(json& j);
  
  /**
   * @brief write configuration to string
   * @param s string path
   **/
  void toString(std::string& s);
  

private:

  enum TRIG_CONF_VALUE m_trigCfg;            //! trigger config
  bool m_verbose;                            //! verbose mode
  bool m_trigEnabled;                        //! trigger is enabled
  uint32_t m_trigMode;                       //! trigger mode
  uint32_t m_trigMask;                       //! trigger mask
  uint32_t m_enableMask;                     //! enable mask   
  uint32_t m_trigCnt;                        //! number of triggers
  uint32_t m_trigTime;                       //! trigger time
  uint32_t m_trigFreq;                       //! trigger frequency
  uint32_t m_trigWordLength;                 //! number of trigger words
  std::vector<uint32_t> m_trigWords;         //! the trigger words
  std::map<uint64_t, std::vector<uint8_t> > m_trigFifo;  //! fifo per elink
  std::map<uint64_t, std::vector<uint8_t> > m_fifo;  //! fifo per elink
  std::vector<const uint8_t*> m_data;        //! buffer of addresses for netio
  std::vector<size_t> m_size;                //! buffer of sizes for netio
  std::map<uint64_t,bool> m_elinks;          //! enabled elinks
  std::map<uint64_t,bool> m_trigElinks;       //! trigger elinks
  std::map<netio::tag,felix::base::ToFELIXHeader> m_headers; //! buffer of headers for netio
  netio::low_latency_send_socket * m_socket; //! netio send socket
  netio::context * m_context;                //! netio underlaying technology
  std::thread m_trigProc;                    //! trigger thread
  std::mutex m_mutex;

  BitStream m_fifoBits;

  void connect();
  void trigger();
  void doTriggerCnt();                       //! loop for a fixed number of triggers
  void doTriggerTime();                      //! loop to trigger during a time slot
  void printFifo(uint64_t elink); 
  uint32_t m_extend;
  bool m_padding;
  bool m_flip;
  bool m_manchester;
  bool m_debug;
  
  void prepareTrigger();
  void prepareFifo(std::vector<uint8_t> *fifo);
  void writeFifo(std::vector<uint8_t> *fifo, uint32_t value);

  std::string m_felixhost;
  uint16_t m_felixport;
  bool m_connected;

  // NetioHandler
  NetioHandler& m_nioh;

};

#endif
