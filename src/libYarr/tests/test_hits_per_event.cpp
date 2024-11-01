#include "catch.hpp"

#include "AllHistogrammers.h"
#include "EventData.h"
#include "Histo1d.h"

TEST_CASE("HistogramHitsPerEvent", "[Histogrammer][HitsPerEvent]") {
    auto algo = StdDict::getHistogrammer("HitsPerEvent");

    REQUIRE (algo);

    auto data = std::make_unique<FrontEndData>();
    data->newEvent(1, 2, 3);
    data->curEvent->addHit(1, 2, 3);

    // Create output histogram
    algo->create(data->lStat);

    algo->processEvent(data.get());

    std::unique_ptr<HistogramBase> result(algo->getHisto());

    REQUIRE (result);

    // Only one thing
    REQUIRE (!algo->getHisto());

    REQUIRE (result->getXaxisTitle() == "Number of Hits");
    REQUIRE (result->getYaxisTitle() == "Events");

    auto histo_as_1d = dynamic_cast<Histo1d *>(&*result);

    REQUIRE (histo_as_1d);

    REQUIRE (histo_as_1d->getEntries() == 1);
    REQUIRE (histo_as_1d->getMean() == 1);

    const unsigned int max = 1000; // Fei4?
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
