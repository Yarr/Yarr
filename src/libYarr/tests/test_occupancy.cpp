#include "catch.hpp"

#include "AllHistogrammers.h"
#include "EventData.h"
#include "Histo2d.h"

TEST_CASE("HistogramOccupancyMap", "[Histogrammer][OccupancyMap]") {
    auto algo = StdDict::getHistogrammer("OccupancyMap");

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

    REQUIRE (result->getXaxisTitle() == "Column");
    REQUIRE (result->getYaxisTitle() == "Row");
    REQUIRE (result->getZaxisTitle() == "Hits");

    auto histo_as_2d = dynamic_cast<Histo2d *>(&*result);

    REQUIRE (histo_as_2d);

    REQUIRE (histo_as_2d->numOfEntries() == 1);
    REQUIRE (histo_as_2d->getMean() == 1);

    auto lowX = histo_as_2d->getXlow();
    auto highX = histo_as_2d->getXhigh();
    auto deltaX = histo_as_2d->getXbinWidth();
    auto lowY = histo_as_2d->getYlow();
    auto highY = histo_as_2d->getYhigh();
    auto deltaY = histo_as_2d->getYbinWidth();

    for(auto x=lowX+deltaX/2; x<highX; x+=deltaX) {
        for(auto y=lowY+deltaY/2; y<highY; y+=deltaY) {
            auto bin = histo_as_2d->binNum(x, y);
            CAPTURE (x, y, bin);

            float val = 0.0f;
            if((int)x == 2 && (int)y == 1) {
                val = 1.0f;
            }
            REQUIRE (histo_as_2d->getBin(bin) == val);
        }
    }
}
