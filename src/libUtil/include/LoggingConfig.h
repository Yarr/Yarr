#ifndef YARR_LOGGING_CONFIG_H
#define YARR_LOGGING_CONFIG_H

#include "storage.hpp"

namespace logging {

/// Setup loggers according to configuration in json file
void setupLoggers(const json &j);

/// List loggers to std::cout, with details of sinks and levels
void listLoggers(bool print_details = false);

}

#endif
