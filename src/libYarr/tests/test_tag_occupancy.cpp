#include "catch.hpp"

#include "AllHistogrammers.h"
#include "EventData.h"
#include "Histo3d.h"

TEST_CASE("HistogramTagOccupancyMap", "[Histogrammer][TagOccupancyMap]") {
    auto algo = StdDict::getHistogrammer("TagOccupancyMap");

    auto data = std::make_unique<FrontEndData>();

    auto tag_limit = GENERATE ( 1, 4, 10 );

    CAPTURE (tag_limit);

    algo->loadConfig(json{{"tag_count", tag_limit}});

    int r = 5;
    int c = 5;
    algo->setMapSize(c, r);

    unsigned tag1 = 2;
    data->newEvent(tag1, 2, 3);
    data->curEvent->addHit(1, 2, 3);

    unsigned tag2 = 5;
    data->newEvent(tag2, 2, 3);
    data->curEvent->addHit(2, 4, 3);

    // Create output histogram
    algo->create(data->lStat);

    algo->processEvent(data.get());

    std::unique_ptr<HistogramBase> result = algo->getHisto();

    REQUIRE (result);

    // Only one thing
    REQUIRE (!algo->getHisto());

    CHECK (result->getXaxisTitle() == "Column");
    CHECK (result->getYaxisTitle() == "Row");
    CHECK (result->getZaxisTitle() == "Tag");

    auto histo_as_3d = dynamic_cast<Histo3d *>(&*result);

    REQUIRE (histo_as_3d);

    CHECK (histo_as_3d->getXbins() == c);
    CHECK (histo_as_3d->getYbins() == r);
    CHECK (histo_as_3d->getZbins() == tag_limit);

    unsigned expect_overflow = 0;
    if(tag_limit < tag2) {
      expect_overflow ++;
    }
    if(tag_limit < tag1) {
      expect_overflow ++;
    }

    // Overflows are counted in entries
    CHECK (histo_as_3d->numOfEntries() == 2);

    CHECK (histo_as_3d->getOverflow() == Catch::Approx(expect_overflow));

    auto bin1 = histo_as_3d->binNum(2, 1, tag1);
    CAPTURE (bin1);

    REQUIRE (histo_as_3d->getBin(bin1) == int{tag_limit > tag1});

    auto bin2 = histo_as_3d->binNum(4, 2, tag2);
    CAPTURE (bin2);

    REQUIRE (histo_as_3d->getBin(bin2) == int{tag_limit > tag2});
}
