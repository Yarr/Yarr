#ifndef STARJSONDATA_H
#define STARJSONDATA_H

// #################################
// # Author: Olivier Arnaez
// # Email: Olivier.Arnaez at cern.ch
// # Project: Yarr
// # Description: Json data Container specific to Star modules, i.e. separating data in vectors for each ABCStar
// # Comment: The format of such data is as follows
// # {
// #  "ChipX": {
// #            "prop1": {
// #                      "prop1a" : {"Data" : [...,...,...]}
// #                      "prop1b" : {"Data" : [...,...,...]}
// #                      ...
// #                    }
// #            "prop2": {
// #                      "prop2a" : {"Data" : [...,...,...]}
// #                      "prop2b" : {"Data" : [...,...,...]}
// #                      ...
// #                    }
// #           ...
// #          }
// #  ...
// # }
// # For instance:
// # {
// #  "Chip0": {
// #             "Row0": {  "Data" : [...,...,...]  }
// #             "Row1": {  "Data" : [...,...,...]  }
// #  }
// #  "Chip1": {
// #             "Row0": {  "Data" : [...,...,...]  }
// #             "Row1": {  "Data" : [...,...,...]  }
// #  }
// #...
// # }
// #################################

#include "JsonData.h"

#include "LoopStatus.h"
#include "storage.hpp"

#include <vector>
#include <string>
#include <optional>

/*! Actual implementation for reading input data from json file on disk, implements methods to do summing of (star) channels */
class StarJsonData : public JsonData
{
public:
       typedef std::vector<std::string> PropName;

       StarJsonData(const std::string &arg_name) : JsonData(arg_name) {}; //!< Constructor with object name without LoopStatus
       StarJsonData(const std::string &arg_name, const LoopStatus &stat) : JsonData(arg_name, stat) {}; //!< Constructor with object name and LoopStatus
       ~StarJsonData() {};

       void setJsonDataType(const std::string & type) //!<Gives a type to the set of data containing in the Json object
       {
              m_jsondata["Type"] = type;
       }; 
       void initialiseStarChannelsDataAtProp(const PropName &propName, const unsigned int nbVals=128); //!<Initializes a vector of nbVals (usually nbChannels per row)
       std::optional<double> getValForProp(const PropName &propName, const unsigned int index) const; //!<Returns the data value at index index for property propName
       std::string getStringForProp(const PropName &propName) const; //!<Returns the string value at property propName
       template<class T> void setValForProp(const PropName &propName, const unsigned int index, const T val); //!<Sets the value val for the item at index in the vector of data at propName
       int getNumberOfEntriesForProp(const PropName &propName) const; //!<Gets the number of entries for a given property
       double getSumOfEntriesForProp(const PropName &propName) const; //!<Gets the sum of all entries for a given property
       double getSumOfSquaredEntriesForProp(const PropName &propName) const; //!<Gets the sum of all squared entries for a given property
       //void plotAs2DMapRowVsAllChannels(const std::string &basename, const std::string &dir = ""); //!<Not mandatory, To be implemented
protected:

private:
};

//! Sets the value val for the item at index in the vector of data at propName
/*!
  \param propName Name of the property in which we'll change the value
  \param index Index of the item in the property we want to change
  \param val New value of the item
*/
template<class T> void StarJsonData::setValForProp(const PropName &propName, const unsigned int index, const T val)
{
       auto splitProp = propName;
       //Getting the element in the json property structure
       auto ref = std::ref(m_jsondata);
       for (std::string i : splitProp)
              ref = ref.get()[i];
       ref = ref.get()["Data"];
       ref.get()[index] = val;
}


#endif

