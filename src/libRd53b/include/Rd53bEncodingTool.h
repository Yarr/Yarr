#ifndef RD53BENCODINGTOOL_H
#define RD53BENCODINGTOOL_H

#include "Rd53bChipMap.h"
#include "EventData.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <array>


#define use64 0

#if use64
typedef uint64_t outWordType;
#else
typedef uint32_t outWordType;
#endif

class Rd53bEncodingTool
{
public:
  Rd53bEncodingTool();
  ~Rd53bEncodingTool(){};

  enum StatusCode
  {
    FAIL,
    SUCCESS
  };

  StatusCode initialize();
  StatusCode saveDataStream();

  Rd53bEncodingTool::StatusCode createStream(Rd53bChipMap &chipmap);
  void setEventsPerStream(int NE) { m_eventsPerStream = NE; }
  void setSuppressEndOfEvent(bool flag) { m_suppressEndOfEvent = flag; }
  void setPlainHitMap(bool flag) { m_plainHitMap = flag; }
  //void setOutputDir(std::string dir) { m_outputDir = dir; }
  void generate(int totEvents, double occupancy = 1e-3, int clusterSize = 1, int orientation = 0);
  int addOrphanBits();
  void setSeed(int seed){m_chipMap->setSeed(seed);}

  //struct hitCoordinate
  //{
  //  int event;
  //  int col;
  //  int row;
  //  uint16_t tot;
  //};
  //std::vector<hitCoordinate> getTruthHits(){return m_truthHits;}
  FrontEndData getTruthData(){return *m_truthData;}
  std::vector<outWordType>   getWords(){return m_outWords;}

private:
  void ATH_MSG_DEBUG(const char *string) { std::cout << string << std::endl; }
  void ATH_MSG_INFO(const char *string) { std::cout << string << std::endl; }

  static int getNumOfOrphanBits(int length, int size = 63);
  std::string getNewTag();

  bool m_addresscompression;
  bool m_compression;
  bool m_suppressToT;
  int m_eventsPerStream;
  bool m_suppressEndOfEvent;
  int m_seed = 0;
  //std::vector<hitCoordinate> m_truthHits;
  std::vector<outWordType> m_outWords;

  //standard interface for decoded or truth hits
  std::unique_ptr<FrontEndData> m_truthData;

  std::unique_ptr<Rd53bChipMap> m_chipMap;

  // for testing the stream creation
  unsigned m_testEvent;
  uint8_t m_streamTag;
  uint8_t m_intTag;
  //std::string m_testFileName;
  std::vector<std::string> m_testStreamsStr;

  //std::ofstream m_testChipFile;
  //std::ofstream m_testStreamBinFile;
  //std::ofstream m_testStreamTXTFile;
  bool m_plainHitMap;
  //std::string m_outputDir;

  bool m_debug;

  // Constants
  static constexpr int NS = 1;
  static constexpr int TagX = 8;
  static constexpr int TagY = 11;
  static constexpr int ccol_bits = 6;
  static constexpr int crow_bits = 8;
  static constexpr int islast_isneighbour = 2;
};

#endif // RD53BENCODINGTOOL_H
