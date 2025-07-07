#include "catch.hpp"

#include "EventData.h"

TEST_CASE("FrontEndData", "[EventData]") {
  FrontEndData ee;
  ee.newEvent(10, 11, 12);
  CHECK ( ee.curEvent->tag == 10 );
  CHECK ( ee.curEvent->l1id == 11 );
  CHECK ( ee.curEvent->bcid == 12 );
}

TEST_CASE("FrontEndDataBenchmark", "[EventData][!benchmark]") {
  FrontEndData ee;
  BENCHMARK("EventData") {
    ee.newEvent(20, 21, 22);
    return ee.curEvent;
  };
}

TEST_CASE("FrontEndEvent", "[EventData]") {
  FrontEndEvent ff(2, 3, 4);

  CHECK ( ff.tag == 2 );
  CHECK ( ff.l1id == 3 );
  CHECK ( ff.bcid == 4 );
  CHECK ( ff.hits.empty() );

  ff.addHit(10, 20, 3);

  CHECK ( ff.hits.size() == 1 );
  // NB row/col reversed compared to addHit
  CHECK ( ff.hits[0] == FrontEndHit{20, 10, 3} );

  FrontEndEvent other(5, 6, 7);

  CHECK ( other.tag == 5 );
  CHECK ( other.l1id == 6 );
  CHECK ( other.bcid == 7 );
  CHECK ( other.hits.empty() );

  other.addHit(1, 2, 1);
  other.addHit(4, 3, 2);

  ff.addEvent(other);

  CHECK ( ff.tag == 2 );
  CHECK ( ff.l1id == 3 );
  CHECK ( ff.bcid == 4 );
  CHECK ( ff.hits.size() == 3 );

  CHECK ( ff.hits[2] == FrontEndHit{3, 4, 2} );
}

TEST_CASE("FrontEndEventBenchmark", "[EventData][!benchmark]") {
  FrontEndEvent ee;
  BENCHMARK("FrontEventEvent") {
    ee.addHit(1, 2, 3);
    return ee.hits.size();
  };
}

TEST_CASE("FrontEndHit", "[EventData]") {
  FrontEndHit hh{2, 3, 4};

  CHECK ( hh == FrontEndHit{2, 3, 4} );
  CHECK ( hh != FrontEndHit{2, 3, 5} );
  CHECK ( hh != FrontEndHit{2, 4, 4} );
  CHECK ( hh != FrontEndHit{3, 3, 4} );
}
