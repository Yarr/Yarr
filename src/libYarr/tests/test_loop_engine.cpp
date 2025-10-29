#include "catch.hpp"

// Direct test of LoopEngine rather than ScanBase
#include "LoopEngine.h"

#include "Bookkeeper.h"
#include "EmptyHw.h"
#include "LoopActionBase.h"

#include <algorithm>
#include <cstddef>
#include <limits>

class TestLoopAction : public LoopActionBase {
  std::vector<std::string> &m_scan_log;
  std::string m_name;
  size_t m_loop_target;

public:
  TestLoopAction(std::vector<std::string> &scan_log, const std::string &name, size_t loops = 0)
    : LoopActionBase(LOOP_STYLE_NOP),
      m_scan_log(scan_log),
      m_name(name),
      m_loop_target(loops)
  {
  }

  void closeOut() override {
    m_scan_log.push_back(m_name+"::closeOut");
  }
  void init() override {
    m_done = false;
    if(m_loop_target == 0) {
      m_done = true;
    }

    m_scan_log.push_back(m_name+"::init");
    g_stat->set(this, 0);
  }
  void end() override {
    m_scan_log.push_back(m_name+"::end");
  }
  void execPart1() override {
    auto stat = g_stat->get(this);
    m_scan_log.push_back(m_name+"::execPart1_"+std::to_string(stat));
  }
  void execPart2() override {
    auto stat = g_stat->get(this);
    m_scan_log.push_back(m_name+"::execPart2_"+std::to_string(stat));

    stat++;
    g_stat->set(this, stat);

    if(stat >= m_loop_target) {
      m_done = true;
    }
  }
};

TEST_CASE("BasicLoopEngine", "[Yarr][LoopEngine]") {
  EmptyHw empty;
  Bookkeeper bookie(&empty, &empty);
  LoopEngine engine(&bookie);

  SECTION("Empty engine runs OK") {
    engine.init();
    engine.execute();
    engine.end();
  }

  SECTION("Check sequence with one layer") {
    std::vector<std::string> scan_log;
    auto test_loop = std::make_shared<TestLoopAction>(scan_log, "A");
    engine.addAction(test_loop);

    engine.init();
    engine.execute();
    engine.end();

    CAPTURE(scan_log.size());

    size_t o=0;
    CHECK(scan_log[o++] == "A::init");
    CHECK(scan_log[o++] == "A::end");
    CHECK(scan_log[o++] == "A::closeOut");
    CHECK(o == scan_log.size());
  }

  SECTION("Check sequence with one layer and loop") {
    std::vector<std::string> scan_log;
    auto test_loop = std::make_shared<TestLoopAction>(scan_log, "A", 1);
    engine.addAction(test_loop);

    engine.init();
    engine.execute();
    engine.end();

    CAPTURE(scan_log.size());
    CAPTURE(scan_log);

    size_t o=0;
    CHECK(scan_log[o++] == "A::init");
    CHECK(scan_log[o++] == "A::execPart1_0");
    CHECK(scan_log[o++] == "A::execPart2_0");
    CHECK(scan_log[o++] == "A::end");
    CHECK(scan_log[o++] == "A::closeOut");
    CHECK(o == scan_log.size());
  }

  SECTION("Check sequence with two layers") {
    std::vector<std::string> scan_log;
    auto test_loop_a = std::make_shared<TestLoopAction>(scan_log, "A", 2);
    auto test_loop_b = std::make_shared<TestLoopAction>(scan_log, "B", 2);
    engine.addAction(test_loop_a);
    engine.addAction(test_loop_b);

    engine.init();
    engine.execute();
    engine.end();

    CAPTURE(scan_log.size());

    size_t o=0;
    CHECK(scan_log[o++] == "A::init");
    CHECK( scan_log[o++] == "A::execPart1_0");
    CHECK(  scan_log[o++] == "B::init");
    CHECK(   scan_log[o++] == "B::execPart1_0");
    CHECK(   scan_log[o++] == "B::execPart2_0");
    CHECK(   scan_log[o++] == "B::execPart1_1");
    CHECK(   scan_log[o++] == "B::execPart2_1");
    CHECK(  scan_log[o++] == "B::end");
    CHECK( scan_log[o++] == "A::execPart2_0");
    CHECK( scan_log[o++] == "A::execPart1_1");
    CHECK(  scan_log[o++] == "B::init");
    CHECK(   scan_log[o++] == "B::execPart1_0");
    CHECK(   scan_log[o++] == "B::execPart2_0");
    CHECK(   scan_log[o++] == "B::execPart1_1");
    CHECK(   scan_log[o++] == "B::execPart2_1");
    CHECK(  scan_log[o++] == "B::end");
    CHECK( scan_log[o++] == "A::execPart2_1");
    CHECK(scan_log[o++] == "A::end");
    CHECK(scan_log[o++] == "A::closeOut");
    CHECK(scan_log[o++] == "B::closeOut");
    CHECK(o == scan_log.size());
  }
}
