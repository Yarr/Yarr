#include "Itkpixv2Encoder.h"
#include <iostream>
#include <memory>
#include <bitset>

#include "AllProcessors.h"

#include "EventData.h"

#include "Itkpixv2Cfg.h"


int main(){
    std::unique_ptr<Itkpixv2Encoder> encoder(new Itkpixv2Encoder());
    encoder->test();

    std::vector<uint32_t> words = encoder->getWords();
    int nWords = words.size();
    std::cout << "There are " << nWords << " words.\n";
    for (uint32_t w : words){
        std::bitset<32> bw(w);
        std::cout << bw << "\n";
    }

    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("ITKPIXV2");

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    Itkpixv2Cfg cfg;  
    proc->connect(&cfg, &rd_cp, &em_cp );

    proc->init();
    std::cout << "1\n";
    proc->run();
    std::cout << "2\n";
    RawDataPtr rd = std::make_shared<RawData>(0, nWords);
    std::cout << "3\n";
    uint32_t *buffer = rd->getBuf();
    buffer[nWords-1] = 0;
    std::cout << "4\n";

    std::copy(words.data(), words.data()+nWords, buffer);
    std::cout << "5\n";
    std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));
    std::cout << "6\n";

    rdc->add(std::move(rd));
    std::cout << "7\n";

    rd_cp.pushData(std::move(rdc));
    std::cout << "8\n";

    rd_cp.finish();
    std::cout << "9\n";

    //proc->join();
    //std::cout << "10\n";
    auto data = em_cp.popData();
    FrontEndData &rawData = *(FrontEndData*)data.get();
    std::cout << "10\n";

    int truthNHits = 0;
    int rawNHits = 0;

    for (int ievt = 0; ievt < rawData.events.size(); ievt++){
	    for(int ihit = 0; ihit < rawData.events[ievt].hits.size(); ihit++){
		    std::cout << rawData.events[ievt].hits[ihit].col << " " << rawData.events[ievt].hits[ihit].row << " " << rawData.events[ievt].hits[ihit].tot << "\n";
            rawNHits++;
	    }
        ievt++;
    }



}