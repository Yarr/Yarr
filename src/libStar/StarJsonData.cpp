// #################################
// // # Author: Olivier Arnaez
// // # Email: Olivier Arnaez at cern.ch
// // # Project: Yarr
// // # Description: Json data Container corresponding to a Star module, i.e. having chips
// // # Comment:
// // ################################
//
#include "StarJsonData.h"

#include <iostream>
#include <iomanip>
#include <fstream>
//#include <boost/algorithm/string.hpp>

//#include "storage.hpp"

#include "logging.h"


namespace
{
auto hlog = logging::make_log("StarJsonData");
}

//! Loads the json data from the pointed input file
/*!
  \param filename Input json filename
*/
void StarJsonData::loadJsonData(const std::string &filename)
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

//! Initializes a vector of nbVals (usually nbChannels per row)
/*!
  \param propName Name of the json property to initialize (hierarchy levels being separated by '/')
  \param nbVals Number of items the property's data will contain
*/
void StarJsonData::initialiseStarChannelsDataAtProp(const std::string propName, const unsigned int nbVals)
{
       hlog->debug("StarJsonData initialising StarChannelsData for prop '{}' with {} vals in the Data vector.", propName, nbVals);
       std::vector<std::string> splitProp;
       boost::split(splitProp, propName, [](char c) { return c == '/'; });
       //Creating the json property structure
       auto level = std::ref(m_jsondata);
       for (unsigned int iPropLev=0; iPropLev<splitProp.size(); iPropLev++)
       {
              std::string strLev = splitProp[iPropLev];
              if (level.get()[strLev].empty())
                     level.get()[strLev] = json::object();
              level = level.get()[strLev];
       }
       //Creating the final data array
       json lastLevel;
       lastLevel["Data"] = json::array();
       for (unsigned int i=0; i<nbVals; i++) 
              lastLevel["Data"].push_back(json::Type::null);
       level.get() = lastLevel;
}

//! Gets the data value at index index for the property propName
/*!
  \param propName Name of the json property (hierarchy levels being separated by '/') we want to retrieve the value from
  \param index Index of the item in the property to retrieve
*/
std::optional<double> StarJsonData::getValForProp(const std::string propName, const unsigned int index) const
{
       std::vector<std::string> splitProp;
       boost::split(splitProp, propName, [](char c) { return c == '/'; });
       //Getting the element in the json property structure
       auto ref = std::ref(m_jsondata);
       for (std::string i : splitProp)
              ref = ref.get()[i];
       ref = ref.get()["Data"];
       if (ref.get()[index].is_null()) return {};
       return ref.get()[index];
}


//! Returns the number of entries in all data for the property
/*!
  \param propName Name of the json property (hierarchy levels being separated by '/') we want the sum of number of entries for
*/
int StarJsonData::getNumberOfEntriesForProp(const std::string propName) const
{
       hlog->debug("StarJsonData counting entries for property {}", propName);
       std::vector<std::string> splitProp;
       boost::split(splitProp, propName, [](char c) { return c == '/'; });
       //Getting the element in the json property structure
       auto ref = std::ref(m_jsondata);
       for (std::string i : splitProp)
              ref = ref.get()[i];

       double entries = 0.;
       if (ref.get().find("Data") != ref.get().end())
       {
              ref = ref.get()["Data"];
              for (unsigned int index=0; index<ref.get().size(); index++)
                     if (!ref.get()[index].is_null()) entries++;
       }
       else
              for (auto it = ref.get().begin(); it != ref.get().end(); ++it)
                     entries += getNumberOfEntriesForProp(propName + "/" + it.key());

       hlog->debug("StarJsonData counted {} entries for property {}", entries, propName);
       return entries;
}

//! Returns the sum of all (sub)entries for the property propName
/*!
  \param propName Name of the json property (hierarchy levels being separated by '/') we want the sum of entries for
*/
double StarJsonData::getSumOfEntriesForProp(const std::string propName) const
{
       hlog->debug("StarJsonData computing sum for {}", propName);
       std::vector<std::string> splitProp;
       boost::split(splitProp, propName, [](char c) { return c == '/'; });
       //Getting the element in the json property structure
       auto ref = std::ref(m_jsondata);
       for (std::string i : splitProp)
              ref = ref.get()[i];

       double sum = 0.;
       if (ref.get().find("Data") != ref.get().end())
       {
              ref = ref.get()["Data"];
              for (unsigned int index=0; index<ref.get().size(); index++)
                     sum += getValForProp(propName, index).value_or(0);
       }
       else
              for (auto it = ref.get().begin(); it != ref.get().end(); ++it)
                     sum += getSumOfEntriesForProp(propName + "/" + it.key());

       hlog->debug("StarJsonData computed a sum of {} for {}", sum, propName);
       return sum;
}

//! Returns the sum of all (sub)entries being squared for the property propName, usefull to later compute std deviation
/*!
  \param propName Name of the json property (hierarchy levels being separated by '/') we want the sum of squares of entries for
*/
double StarJsonData::getSumOfSquaredEntriesForProp(const std::string propName) const
{
       hlog->debug("StarJsonData computing sum of squares for {}", propName);
       std::vector<std::string> splitProp;
       boost::split(splitProp, propName, [](char c) { return c == '/'; });
       //Getting the element in the json property structure
       auto ref = std::ref(m_jsondata);
       for (std::string i : splitProp)
              ref = ref.get()[i];

       double sum = 0.;
       if (ref.get().find("Data") != ref.get().end())
       {
              ref = ref.get()["Data"];
              for (unsigned int index=0; index<ref.get().size(); index++)
              {
                     double val = getValForProp(propName, index).value_or(0);
                     sum += val*val;
              }
       }
       else
              for (auto it = ref.get().begin(); it != ref.get().end(); ++it)
                     sum += getSumOfSquaredEntriesForProp(propName + "/" + it.key());

       hlog->debug("StarJsonData computed a sum of squares = {} for {}", sum, propName);
       return sum;
}

