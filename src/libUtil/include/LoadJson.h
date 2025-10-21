#ifndef YARR_LOAD_JSON_H
#define YARR_LOAD_JSON_H

#include <string>

#include "storage.hpp"

namespace JsonHelper {

/// Open file and parse into json object.
/**
 * If the name contains '#' the string following this is used as a json_pointer.
 *
 * For example:
 *   configs/full_config.json#/module_0
 */
json openJsonFile(const std::string& filepath);

}

#endif
