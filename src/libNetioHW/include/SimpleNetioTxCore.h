#ifndef SIMPLENETIOTXCORE_H
#define SIMPLENETIOTXCORE_H

/********************************
 * SimpleNetioTxCore
 * Author: Carlos.Solans@cern.ch
 * Description: NetIO Transmitter
 * Date: June 2017
 *********************************/

#include "TxCore.h"

#include "netio/netio.hpp"
#include "felixbase/client.hpp"

#include <cstdint>
#include <queue>
#include <vector>
#include <mutex>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

/**
 * @brief Simple implementation of the NetIO TxCore.
 * This class creates a single socket connection to Felix.
 * The host and port can be defined with the methods
 * SimpleNetioTxCore::parseJson or SimpleNetioTxCore::parseString.
 * Enabling and disabling of the read out of a channel is done
 * with the SimpleNetioTxCore::enableChannel SimpleNetioTxCore::disableChannel.
 * Each channel has an independent FIFO that can be addressed with
 * SimpleNetioTxCore::writeFifo. Commands are sent to the FrontEnd
 * with the SimpleNetioTxCore::releaseFifo methods.
 **/

class SimpleNetioTxCore : virtual public TxCore {
public:

  /**
   * @brief Default constructor.
   * Create NetIO context and low_latency_send_socket.
   **/
  SimpleNetioTxCore();

  /**
   * @brief Destructor
   * Join the trigger thread, and close the socket
   **/
  ~SimpleNetioTxCore();

  /**
   * @brief append to fifo
   * @param value command
   **/
  void writeFifo(uint32_t value) override;

  /**
   * @brief release the fifo for all enabled channels
   **/
  void releaseFifo() override;

  void setCmdEnable(uint32_t) override;
  uint32_t getCmdEnable() override;

  /**
   * @brief check if the fifo of commands is empty
   * @return bool true if fifo is empty
   **/
  bool isCmdEmpty() override;

  /**
   * @brief enable the trigger
   * @param value value of the trigger
   **/
  void setTrigEnable(uint32_t value) override;

  /**
   * @brief get if the trigger is enabled
   * @return uint32_t get trigger enable
   **/
  uint32_t getTrigEnable() override;

  void maskTrigEnable(uint32_t value, uint32_t mask) override;

  /**
   * @brief check if the trigger is done
   * @return bool is trigger done
   **/
  bool isTrigDone() override;

  /**
   * @brief set the trigger config
   * @param cfg TRIG_CONF_VALUE config value
   **/
  void setTrigConfig(enum TRIG_CONF_VALUE cfg) override;

  /**
   * @brief set the trigger frequency
   * @param freq frequency in Hz
   **/
  void setTrigFreq(double freq) override;

  /**
   * @param count the number of desired triggers
   **/
  void setTrigCnt(uint32_t count) override;

  /**
   * @param time the duration of the trigger in seconds
   **/
  void setTrigTime(double time) override;

  /**
   * @param length length of the trigger word starting at MSB
   **/
  void setTrigWordLength(uint32_t length) override;

  /**
   * @param word pointer to trigger words
   * @param size the number of words
   **/
  void setTrigWord(uint32_t *word, uint32_t size) override;

  /**
   * @brief abort the trigger
   **/
  void toggleTrigAbort() override;

  /**
   * @brief set the trigger logic mask
   * @param mask mask of the trigger logic
   **/
  void setTriggerLogicMask(uint32_t mask) override;

  /**
   * @brief set the trigger logic mode
   * @param mode TRIG_LOGIC_MODE_VALUE trigger logic mode
   **/
  void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override;

  /**
   * @brief reset the trigger logic
   **/
  void resetTriggerLogic() override;

  /**
   * @brief get the number of input triggers
   * @return uint32_t the number of trigger counts
   **/
  uint32_t getTrigInCount() override;

  /**
   * @brief read configuration from json
   * @param j reference to json
   **/
  void fromFileJson(json& j);

  /**
   * @brief write configuration to json
   * @param j reference to json
   **/
  void toFileJson(json& j);

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
  std::vector<uint8_t> m_fifo;               //! fifo of commands
  std::vector<const uint8_t*> m_data;        //! buffer of addresses for netio
  std::vector<size_t> m_size;                //! buffer of sizes for netio
  std::map<netio::tag,uint32_t> m_elinks;    //! enabled links
  std::map<netio::tag,uint32_t> m_trigChns;  //! trigger channels
  std::map<netio::tag,felix::base::ToFELIXHeader> m_headers; //! buffer of headers for netio
  netio::low_latency_send_socket * m_socket; //! netio send socket
  netio::context * m_context;                //! netio underlaying technology
  std::thread m_trigProc;                    //! trigger thread
  std::mutex m_mutex;

  void connect();
  void doTriggerCnt();                       //! loop for a fixed number of triggers
  void doTriggerTime();                      //! loop to trigger during a time slot
  void printFifo();

  void releaseFifo(bool trigChns);

  void enableChannel(uint64_t channel);
  void disableChannel(uint64_t channel);

  uint32_t m_elinkSize;
  bool m_padding;
  bool m_flip;
  bool m_debug;
  bool m_trigAbort;

  std::string m_felixhost;
  uint16_t m_felixport;
  bool m_connected;

};

#endif
