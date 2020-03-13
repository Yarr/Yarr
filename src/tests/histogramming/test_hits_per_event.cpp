#include "catch.hpp"

#include "AllHistogrammers.h"
#include "Fei4EventData.h"
#include "Histo1d.h"

// Strictly, it's in libFei4, but not specifically
TEST_CASE("HistogramHitsPerEvent", "[Histogrammer][Fei4][notFei4][HitsPerEvent]") {
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

    {
        auto data = std::make_unique<Fei4Data>();
        data->newEvent(1, 2, 3);
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

    REQUIRE (histo_as_1d->getEntries() == 1);
    REQUIRE (histo_as_1d->getMean() == 1);

    const unsigned int max = 16; // Fei4?
    REQUIRE (histo_as_1d->size() == max);

    auto lowX = -0.5;
    auto highX = max-0.5;
    auto deltaX = 1.0;

    for(auto x=lowX+deltaX/2; x<highX; x+=deltaX) {
        auto bin = x; // histo_as_1d->getBin(x);
        CAPTURE (x, bin);

        float val = 0.0f;
        if((int)x == 1) {
            val = 1.0f;
        }
        REQUIRE (histo_as_1d->getBin(bin) == val);
    }
}
