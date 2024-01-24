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

    // the following 3 examples result in the same histograms
    { // 1 FrontEndData with 2 events
        auto data = std::make_unique<FrontEndData>();
        // each event with 1 hit
        data->newEvent(1, 2, 3);
        data->curEvent->addHit(1, 2, 3);
        data->newEvent(1, 10, 3);
        data->curEvent->addHit(1, 5, 3);
        input.pushData(std::move(data)); // data is by default end_of_iteration
    }

    { // 2 FrontEndData with 1 event each
        auto data1 = std::make_unique<FrontEndData>();
        data1->newEvent(1, 2, 3);
        data1->curEvent->addHit(1, 2, 3);
        data1->lStat.is_end_of_iteration = false;
        input.pushData(std::move(data1));

        auto data2 = std::make_unique<FrontEndData>();
        //data2->lStat.is_end_of_iteration = true; // it is true by default
        data2->newEvent(1, 10, 3);
        data2->curEvent->addHit(1, 5, 3);
        input.pushData(std::move(data2));
    }

    { // 2 FrontEndData with events, and 1 empty FrontEndData as the marker of the iteration end
        auto data1 = std::make_unique<FrontEndData>();
        data1->lStat.is_end_of_iteration = false;
        data1->newEvent(1, 2, 3);
        data1->curEvent->addHit(1, 2, 3);
        input.pushData(std::move(data1));

        auto data2 = std::make_unique<FrontEndData>();
        data2->lStat.is_end_of_iteration = false;
        data2->newEvent(5, 3, 3);
        data2->curEvent->addHit(7, 8, 10);
        input.pushData(std::move(data2));

        auto data_end = std::make_unique<FrontEndData>();
        // empty FrontEndData, but it marks the iteration end
        data_end->lStat.is_end_of_iteration = true;
        input.pushData(std::move(data_end));

    }

    input.finish();
    histogrammer.join();

    REQUIRE (!output.empty());

    int n_iterations = 3;
    while (n_iterations-- > 0) {
        std::unique_ptr<HistogramBase> result = output.popData();

        REQUIRE (result->getXaxisTitle() == "Number of Hits");
        REQUIRE (result->getYaxisTitle() == "Events");

        auto histo_as_1d = dynamic_cast<Histo1d *>(&*result);

        REQUIRE (histo_as_1d);

        REQUIRE (histo_as_1d->getEntries() == 2); // 2 events
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
            if((int)x == 1) { // with 1 hit
                val = 2.0f;   // 2 events
            }

            REQUIRE (histo_as_1d->getBin(bin) == val);
        }
    }

    // only 3 histogram outputs
    REQUIRE (output.empty());
}
