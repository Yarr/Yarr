/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2015-Jul-21
 */

#include "StdDataGatherer.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <set>
#include <csignal>

#include "logging.h"

namespace {
    auto sdglog = logging::make_log("StdDataGatherer");
}

StdDataGatherer::StdDataGatherer() : LoopActionBase(LOOP_STYLE_DATA) {
    loopType = typeid(this);
    min = 0;
    max = 0;
    step = 1;
    counter = 0;
}

void StdDataGatherer::init() {
    m_done = false;
    killswitch = false;
    SPDLOG_LOGGER_TRACE(sdglog, "");
}

void StdDataGatherer::end() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
}

void StdDataGatherer::execPart1() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
    if (g_tx->getTrigEnable() == 0)
        SPDLOG_LOGGER_ERROR(sdglog, "Trigger is not enabled, will get stuck here!");

}

sig_atomic_t signaled = 0;

void StdDataGatherer::execPart2() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
    unsigned count = 0;
    bool done = false;
    unsigned n_all_rx_read_iterations = 0;
    uint32_t n_all_received_triggers = 0;

    signaled = 0;
    signal(SIGINT, [](int signum){signaled = 1;});
    signal(SIGUSR1, [](int signum){signaled = 1;});

    // the RX channels that actually received data, and expect feedback                                                                                                     
    std::set<uint32_t> activeChannels;

    // the counters for the feedback from data processors                                                                                                                   
    std::map<uint32_t, uint32_t> channelReceivedTriggersCnt;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
      channelReceivedTriggersCnt[id] = 0;
    }

    SPDLOG_LOGGER_WARN(sdglog, "IMPORTANT! Going into endless loop unless timelimit is set, interrupt with ^c (SIGINT)!");

    std::vector<RawDataPtr> newData;
    while (!done) {
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;
        
        done = g_tx->isTrigDone();
        newData =  g_rx->readData();
        
        // Read all data until buffer is empty
        while (newData.size() > 0 && count < 4096 && signaled == 0 && !killswitch) {
	    n_all_rx_read_iterations++;
            if (newData.size() > 0) {
                for (auto &dataChunk : newData) {
                    count += dataChunk->getSize();
                    for (unsigned &uid : keeper->getRxToId(dataChunk->getAdr())) {
                        if (rdcMap[uid] == nullptr) {
                            rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                        }

                        rdcMap[uid]->add(dataChunk);
                    }
                }
            }
            // Wait a little bit to increase chance of new data having arrived
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            newData =  g_rx->readData();
        }

        for (auto &[id, rdc] : rdcMap) {
            keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
	    activeChannels.insert(id);
        }
        

	// check for any feedback from data processing                                                                                                                      
	bool received_feedback_somewhere = false;
        do {
	  received_feedback_somewhere = false;

	  // check the active channels for feedback                                                                                                                        
	  for (auto& chan_id : activeChannels) {
	    // pull all the currently available feedback from this channel                                                                                               
	    while(bool received_on_chan = keeper->getEntry(chan_id).fe->clipProcFeedback.waitNotEmptyOrDoneOrTimeout(m_dataProcessingTime)) {
	      received_feedback_somewhere |= received_on_chan;
	      auto params = keeper->getEntry(chan_id).fe->clipProcFeedback.popData();

	      if (params->trigger_tag >=  0) {
		channelReceivedTriggersCnt[chan_id] += 1;
	      }
	    }
	  }
	} while (received_feedback_somewhere);

	// check number of received triggers                                                                                                                   
        for (auto &[id, received_triggers] : channelReceivedTriggersCnt) {
	  //SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 : chan {} received {} triggers from {}", id, received_triggers, n_triggersToReceive);                  
	  n_all_received_triggers += received_triggers;
	}

	if (count == 0) {
          SPDLOG_LOGGER_INFO(sdglog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", count, n_all_rx_read_iterations);
        } else {
          SPDLOG_LOGGER_INFO(sdglog, "--> Received {} words in {} iterations with {} triggers!", count, n_all_rx_read_iterations, n_all_received_triggers);
        }

        count = 0;

        if (signaled == 1 || killswitch) {
	  SPDLOG_LOGGER_WARN(sdglog, "Caught interrupt, stopping data taking!");
	  SPDLOG_LOGGER_WARN(sdglog, "Abort might leave data in buffers!");
	  g_tx->toggleTrigAbort();
	}
	std::this_thread::sleep_for(g_rx->getWaitTime());
    }

    // the iteration end marker for the processing & analysis
    // send end-of-iteration empty container with LoopStatus::is_end_of_iteration = true
    LoopStatus loop_status_iteration_end({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
    loop_status_iteration_end.is_end_of_iteration = true;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
      std::unique_ptr<RawDataContainer> c_iter_end = std::make_unique<RawDataContainer>(std::move(loop_status_iteration_end));
      keeper->getEntry(id).fe->clipRawData.pushData(std::move(c_iter_end));
    }

    m_done = true;
    counter++;
}
