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

#include "netio.hpp"
#include "felixbase/client.hpp"

#include <cstdint>
#include <vector>

#include "storage.hpp"

/**
 * @brief Implementation of the RxCore for NetIO communication.
 *        This class subscribes to felix using a low_latency_socket.
 **/
class NetioTxCore : virtual public TxCore {
public:

  NetioTxCore(); 		// Create NetIO context and low_latency_send_socket
  ~NetioTxCore() override; 		// Delete socket and context.
  void writeFifo(uint32_t value) override; 	// append to fifo of all channels
  void releaseFifo() override; 		// release the fifo for all enabled channels

  void setCmdEnable(uint32_t) override;
  void setCmdEnable(std::vector<uint32_t> channels) override;
  void disableCmd() override;
  uint32_t getCmdEnable() override;
  bool isCmdEmpty() override; 		// check if the fifo of commands is empty
  void setTrigEnable(uint32_t value) override; 	// enable the trigger
  uint32_t getTrigEnable() override; 	// get if the trigger is enabled

  void maskTrigEnable(uint32_t value, uint32_t mask) override;
  bool isTrigDone() override; 		// check if the trigger is done
  void setTrigConfig(enum TRIG_CONF_VALUE cfg) override; 	// set the trigger config
  void setTrigFreq(double freq) override; 	// set the trigger frequency
  void setTrigCnt(uint32_t count) override; 	// set the number of desired triggers
  void setTrigTime(double time) override; 	// set the trigger time in seconds
  void setTrigWordLength(uint32_t length) override; 	// set Trigger Word Length
  void setTrigWord(uint32_t *words, uint32_t size) override; 	// set the trigger words

  void toggleTrigAbort() override; 	// abort the trigger sequence
  void setTriggerLogicMask(uint32_t mask) override; 	// set the trigger logic
  void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override; 	// set the trigger logic mode
  void resetTriggerLogic() override; 	// reset the trigger logic
  uint32_t getTrigInCount() override; 	// get the number of triggers in
  void loadConfig(const json &j); 		// read configuration from json
  void writeConfig(json& j); 		// write configuration to json

private:
  std::string m_feType; // flag used to keep rd53a and strips specific stuff seperate

  enum TRIG_CONF_VALUE m_trigCfg;            //! trigger config
  bool m_trigEnabled;                        //! trigger is enabled
  uint32_t m_trigMode;                       //! trigger mode
  uint32_t m_trigCnt;                        //! number of triggers
  uint32_t m_trigTime;                       //! trigger time
  uint32_t m_trigFreq;                       //! trigger frequency
  uint32_t m_trigWordLength;                 //! number of trigger words
  bool     m_pixFwTrigger;                          //! Cal+trigger from F/W 
  std::vector<uint32_t> m_trigWords;         //! the trigger words
  std::map<uint32_t, std::vector<uint8_t> > m_trigFifo;  //! fifo per elink
  std::map<uint32_t, std::vector<uint8_t> > m_fifo;  //! fifo per elink
  std::vector<const uint8_t*> m_data;        //! buffer of addresses for netio
  std::vector<size_t> m_size;                //! buffer of sizes for netio
  std::map<uint32_t,bool> m_elinks;          //! enabled elinks
  std::map<netio::tag,felix::base::ToFELIXHeader> m_headers; //! buffer of headers for netio
  netio::low_latency_send_socket * m_socket; //! netio send socket
  netio::context * m_context;                //! netio underlaying technology
  std::thread m_trigProc;                    //! trigger thread

  void connect();
  void trigger();
  void doTriggerCnt();                       //! loop for a fixed number of triggers
  void doTriggerTime();                      //! loop to trigger during a time slot
  void printFifo(uint32_t elink);
  void sendFifo(); //! Prepare packet from FIFO and send it through socket.
  uint32_t m_extend;
  bool m_padding;
  bool m_flip;
  bool m_manchester;
  int m_bufferSize;
  bool m_debug;

  void enableChannel(uint32_t channel);
  void disableChannel(uint32_t channel);
  void disableAllChannels();
  void writeFifo(uint32_t channel, uint32_t value);

  void prepareTrigger();
  void prepareFifo(std::vector<uint8_t> *fifo) const;
  void writeFifo(std::vector<uint8_t> *fifo, uint32_t value) const;

  std::string m_felixhost;
  uint16_t m_felixport;
  bool m_connected;

};

#endif
