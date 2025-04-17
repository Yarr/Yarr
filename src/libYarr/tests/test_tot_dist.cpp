#include "catch.hpp"

#include "AllHistogrammers.h"
#include "EventData.h"
#include "Histo1d.h"

TEST_CASE("HistogramTotDist", "[Histogrammer][TotDist]") {
    // This is for one FE
    auto algo = StdDict::getHistogrammer("TotDist");

    auto data = std::make_unique<FrontEndData>();
    data->newEvent(1, 2, 3);
    data->curEvent->addHit(1, 2, 3);

    // Create output histogram
    algo->create(data->lStat);

    algo->processEvent(data.get());

    std::unique_ptr<HistogramBase> result = algo->getHisto();

    REQUIRE (result);

    // Only one thing
    REQUIRE (!algo->getHisto());

    REQUIRE (result->getXaxisTitle() == "ToT [bc]");
    REQUIRE (result->getYaxisTitle() == "# of Hits");
    // REQUIRE (result->getZaxisTitle() == "z");

    auto histo_as_1d = dynamic_cast<Histo1d *>(&*result);

    REQUIRE (histo_as_1d);

    REQUIRE (histo_as_1d->getEntries() == 1);
    REQUIRE (histo_as_1d->getMean() == 3);

    const unsigned int max = 16; // Fei4?
    REQUIRE (histo_as_1d->size() == max);

    auto lowX = -0.5;
    auto highX = max-0.5;
    auto deltaX = 1.0;

    for(auto x=lowX+deltaX/2; x<highX; x+=deltaX) {
        auto bin = x; //  histo_as_1d->getBin(x);
        CAPTURE (x, bin);

        float val = 0.0f;
        if((int)x == 2) {
          val = 1.0f;
        }
        REQUIRE (histo_as_1d->getBin(bin) == val);
    }
}
