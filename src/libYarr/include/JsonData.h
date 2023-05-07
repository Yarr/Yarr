#ifndef JSONDATA_H
#define JSONDATA_H

// #################################
// # Author: Olivier Arnaez
// # Email: Olivier.Arnaez at cern.ch
// # Project: Yarr
// # Description: Json data Container
// # Comment:
// ################################

#include <string>

#include "HistogramBase.h"

#include "LoopStatus.h"
#include "storage.hpp"


/*! Base class for reading input data from json file on disk (actual implementation left to specific json data formats readers) */
class JsonData : public HistogramBase
{
public:
    JsonData(const std::string &arg_name); //!< Constructor with object name
    JsonData(const std::string &arg_name, const LoopStatus &stat); //!< Constructor with object name and LoopStatus
    ~JsonData() = default;

    void toStream(std::ostream &out) const override {};
    void toJson(json &j) const override  //!< Required by base class, returns the current object's data
    {
        j = m_jsondata;
    }; 
    void toFile(const std::string &basename, const std::string &dir = "", bool header= true) const override; //!< Writes the json data to file
    void plot(const std::string &basename, const std::string &dir = "") const override {}; //!< Could be used to create a plot from json data
    void loadJsonData(const std::string &filename); //Loads the json data from the pointed input file
    void setJson(json j) //!< Replaces the object's data with the json object passed as argument
    {
        m_jsondata=j;
    }; 

protected:
    json m_jsondata;
};
#endif
