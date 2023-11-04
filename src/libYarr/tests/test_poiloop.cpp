#include "catch.hpp"

#include "LoopActionBase.h"
#include "StdParameterLoop.h"
#include "AnalysisAlgorithm.h"

namespace {

/// A parameter loop action that has no name
class MyAnonParameterLoop : public LoopActionBase {
  public:
    MyAnonParameterLoop() : LoopActionBase(LOOP_STYLE_PARAMETER) { }
};

/// A parameter loop action that does have a name (via StdParameterAction)
class MyNamedParameterAction : public LoopActionBase, public StdParameterAction {
  public:
    MyNamedParameterAction(const std::string &name)
        : LoopActionBase(LOOP_STYLE_PARAMETER) {
      parName = name;
    }
};

/// A loop action that is not a parameter
class MyNoopLoop : public LoopActionBase {

  public:
    MyNoopLoop() : LoopActionBase(LOOP_STYLE_NOP) {}
    ~MyNoopLoop() {}
};

/// Analysis implementation with access to poi 
class MyAnalysis : public AnalysisAlgorithm {
  public:
    void setPOI(const std::vector<std::string> &poi) {
      m_parametersOfInterest = poi;
    }

    /// Normally protected, make it accessible
    using AnalysisAlgorithm::isPOILoop;
};

} // End anon namespace

bool isPOILoop(MyAnalysis &a, LoopActionBase &l) {
  return a.isPOILoop(&l);
}

/// A test that runs through the different cases is isPOILoop
TEST_CASE("AnalysisPOILoopChecks", "[Analysis]") {
  MyAnalysis aa;

  MyNoopLoop noopLoop;
  CHECK (!isPOILoop(aa, noopLoop));

  MyAnonParameterLoop anonLoop;
  CHECK (isPOILoop(aa, anonLoop));

  StdParameterLoop namedLoop1;
  {
    json name1;
    name1["parameter"] = "Param1";
    namedLoop1.loadConfig(name1);
  }
  StdParameterLoop namedLoop2;
  {
    json name2;
    name2["parameter"] = "Param2";
    namedLoop2.loadConfig(name2);
  }

  // Empty list
  CHECK (isPOILoop(aa, namedLoop1));
  CHECK (isPOILoop(aa, namedLoop2));

  aa.setPOI({"OtherParam"});

  // With a non-empty list find neither loop is of interest
  CHECK (!isPOILoop(aa, namedLoop1));
  CHECK (!isPOILoop(aa, namedLoop2));

  aa.setPOI({"Param1"});
  CHECK (isPOILoop(aa, namedLoop1));
  CHECK (!isPOILoop(aa, namedLoop2));

  aa.setPOI({"Param1", "Param2"});

  CHECK (isPOILoop(aa, namedLoop1));
  CHECK (isPOILoop(aa, namedLoop2));

  aa.setPOI({});

  {
    MyNamedParameterAction namedLoop1("Param1");
    MyNamedParameterAction namedLoop2("Param2");

    // Empty list
    CHECK (isPOILoop(aa, namedLoop1));
    CHECK (isPOILoop(aa, namedLoop2));

    aa.setPOI({"OtherParam"});

    // With a non-empty list find neither loop is of interest
    CHECK (!isPOILoop(aa, namedLoop1));
    CHECK (!isPOILoop(aa, namedLoop2));

    aa.setPOI({"Param1"});
    CHECK (isPOILoop(aa, namedLoop1));
    CHECK (!isPOILoop(aa, namedLoop2));

    aa.setPOI({"Param1", "Param2"});

    CHECK (isPOILoop(aa, namedLoop1));
    CHECK (isPOILoop(aa, namedLoop2));
  }
}
