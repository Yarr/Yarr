#include "catch.hpp"

#include "AllHistogrammers.h"
#include "EventData.h"
#include "Histo1d.h"

//! the goal is to test the operation of the histogrammer: it must aggregate histograms and publish the result only on LoopStatus END
TEST_CASE("HistogrammerOperation", "[Histogrammer][Operation]") {
    // This is for one FE
    std::unique_ptr<DataProcessor> histo(new HistogrammerProcessor);
    auto& histogrammer = static_cast<HistogrammerProcessor&>(*histo);

    ClipBoard<EventDataBase> input;
    ClipBoard<HistogramBase> output;

    histogrammer.connect(&input, &output);

    histogrammer.addHistogrammer(StdDict::getHistogrammer("HitsPerEvent"));
    // histogrammer.addHistogrammer(new HitsPerEvent());

    histogrammer.init();
    histogrammer.run();

    { // empty event
        auto data = std::make_unique<FrontEndData>();
        // 1 event with 0 hits
        data->newEvent(1, 2, 3);
        input.pushData(std::move(data));
    }

    { // a couple events in one data
        auto data = std::make_unique<FrontEndData>();
        // 2 events with 1 hit each
        data->newEvent(1, 2, 3);
        data->curEvent->addHit(1, 2, 3);
        data->newEvent(1, 10, 3);
        data->curEvent->addHit(1, 5, 3);
        input.pushData(std::move(data));
    }

    { // final event data
        auto data = std::make_unique<FrontEndData>();
        // event with 2 hits
        data->newEvent(1, 1, 1);
        data->curEvent->addHit(1, 1, 1);
        data->curEvent->addHit(1, 2, 3);
        input.pushData(std::move(data));
    }

    { // marker of the end of iteration, the data in the marker is ignored
        LoopStatus end_of_processing({0}, {LOOP_STYLE_DATA});
        end_of_processing.is_end_of_iteration = true;
        auto data = std::make_unique<FrontEndData>(end_of_processing);
        // event with 2 hits
        data->newEvent(1, 1, 1);
        data->curEvent->addHit(1, 1, 1); // these hits won't be analysed
        data->curEvent->addHit(1, 2, 3);
        input.pushData(std::move(data));
    }

    input.finish();
    histogrammer.join();

    REQUIRE (!output.empty());

    std::unique_ptr<HistogramBase> result = output.popData();

    // Only one thing
    REQUIRE (output.empty());

    REQUIRE (result->getXaxisTitle() == "Number of Hits");
    REQUIRE (result->getYaxisTitle() == "Events");

    auto histo_as_1d = dynamic_cast<Histo1d *>(&*result);

    REQUIRE (histo_as_1d);

    REQUIRE (histo_as_1d->getEntries() == 4); // 4 events
    REQUIRE (histo_as_1d->getMean() == 1.0);

    const unsigned int max = 1000; // Fei4?
    REQUIRE (histo_as_1d->size() == max);

    auto lowX = -0.5;
    auto highX = max-0.5;
    auto deltaX = 1.0;

    for(auto x=lowX+deltaX/2; x<highX; x+=deltaX) {
        auto bin = x; // histo_as_1d->getBin(x);
        CAPTURE (x, bin);

        float val = 0.0f;
        if((int)x == 0) { // with 0 hit
            val = 1.0f;   // 1 events
        }
        if((int)x == 1) { // with 1 hit
            val = 2.0f;   // 2 events
        }
        if((int)x == 2) { // with 2 hits
            val = 1.0f;   // 1 event
        }

        REQUIRE (histo_as_1d->getBin(bin) == val);
    }
}
