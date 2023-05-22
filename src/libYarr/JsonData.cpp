// #################################
// # Author: Olivier Arnaez
// # Email: Olivier Arnaez at cern.ch
// # Project: Yarr
// # Description: Json data Container
// # Comment:
// ################################

#include "JsonData.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "storage.hpp"

#include "logging.h"

namespace
{
auto hlog = logging::make_log("JsonData");
}

//! Constructor with LoopStatus
/*!
  \param arg_name Name of the object in memory.
  \param stat LoopStatus containing info about the position of the object in the scan hierarchy
*/
JsonData::JsonData(const std::string &arg_name, const LoopStatus &stat)
       : HistogramBase(arg_name, stat)
{
       m_jsondata["Name"] = name;
       for (unsigned i=0; i<stat.size(); i++)
              m_jsondata["loopStatus"][i] = (stat.get(i));
}

//! Constructor without LoopStatus
/*!
  \param arg_name Name of the object in memory.
*/
JsonData::JsonData(const std::string &arg_name)
       : HistogramBase(arg_name)
{
       m_jsondata["Name"] = name;
}

//! Loads the json data from the pointed input file
/*!
  \param filename Input json filename
*/
void JsonData::loadJsonData(const std::string &filename)
{
       std::ifstream file(filename, std::fstream::in);
       try
       {
              if (!file)
              {
                     throw std::runtime_error("could not open file");
              }
              try
              {
                     m_jsondata = json::parse(file);
              }
              catch (json::parse_error &e)
              {
                     throw std::runtime_error(e.what());
              }
       }
       catch (std::runtime_error &e)
       {
              hlog->error("Error opening json file: {}", e.what());
       }
}

//! Function writing json data to the mentioned output location
/*!
  \param prefix Prefix to be added to the object name
  \param dir Output directory
  \param unused
*/
void JsonData::toFile(const std::string &prefix, const std::string &dir, bool /*jsonType*/) const
{
       hlog->info("JsonData::toFile({}, {}) writing with HistogramBase::name {}", prefix, dir, HistogramBase::name);
       json jsonDataToWrite = m_jsondata;
       jsonDataToWrite["FEname"] = prefix;
       std::string filename = dir + prefix + "_" + HistogramBase::name;
       for (unsigned i=0; i<lStat.size(); i++)
              filename += "_" + std::to_string(lStat.get(i));
       filename += ".json";

       std::fstream file(filename, std::fstream::out | std::fstream::trunc);
       file << std::setw(4) << jsonDataToWrite;
       file.close();
}

