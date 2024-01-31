#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Rd53bCfg.h"

#include "Rd53bEncodingTool.h"

//#include "rd53b_test_stream.h"
//#include "rd53b_test_truth.h"

TEST_CASE("Rd53bDataProcessor", "[rd53b][data_processor]") {
  
  //Initialize the Rd53b generator/encoder, give it the random seed,
  //number of events per stream and generate desired number of events

  std::unique_ptr<Rd53bEncodingTool> encoder(new Rd53bEncodingTool());
  encoder->setSeed(0);
  encoder->setEventsPerStream(16);
  encoder->generate(28);

  //Retrieve the truth hits and the encoded words

  FrontEndData          truth    = encoder->getTruthData();
  std::vector<uint32_t> words = encoder->getWords();
  int nWords = words.size();
  
  std::cout << "RD53b processor test\n";
  std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");

  REQUIRE (proc);

  ClipBoard<RawDataContainer> rd_cp;
  ClipBoard<EventDataBase> em_cp;

  Rd53bCfg cfg;  

  proc->connect(&cfg, &rd_cp, &em_cp );

  proc->init();
  proc->run();

  RawDataPtr rd = std::make_shared<RawData>(0, nWords);
  uint32_t *buffer = rd->getBuf();
  buffer[nWords-1] = 0;

  std::copy(words.data(), words.data()+nWords, buffer);


  std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));
  rdc->add(std::move(rd));
  rd_cp.pushData(std::move(rdc));

  rd_cp.finish();

  proc->join();

  REQUIRE (!em_cp.empty());

  auto data = em_cp.popData();
  FrontEndData &rawData = *(FrontEndData*)data.get();

  //REQUIRE (rawData.events.size() == truth_nEvents);
  REQUIRE (rawData.events.size() == truth.events.size());

  int truthNHits = 0;
  int rawNHits = 0;

  for (int ievt = 0; ievt < rawData.events.size(); ievt++){
	  for(int ihit = 0; ihit < rawData.events[ievt].hits.size(); ihit++){
      std::cout << "Attempt " << ihit << "\n";
		  REQUIRE(rawData.events[ievt].hits[ihit].col == truth.events[ievt].hits[ihit].col);
		  REQUIRE(rawData.events[ievt].hits[ihit].row == truth.events[ievt].hits[ihit].row);
		  REQUIRE(rawData.events[ievt].hits[ihit].tot == truth.events[ievt].hits[ihit].tot);
      rawNHits++;
	  }
    truthNHits += truth.events[ievt].nHits;
    ievt++;
  }

  REQUIRE (rawNHits == truthNHits);

  // Only one thing
  REQUIRE (em_cp.empty());
}
