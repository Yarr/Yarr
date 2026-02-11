#ifndef _STORAGE_HPP_
#define _STORAGE_HPP_

// Include more info in exceptions
#define JSON_DIAGNOSTICS 1

#ifdef __GNUC__
// Ignore false positives related to JSON_DIAGNOSTICS and gcc 14
// (see https://github.com/nlohmann/json/issues/3808)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#include <json.hpp>
#pragma GCC diagnostic pop
#else
#include <json.hpp>
#endif

// Mostly the defaults, but using 32 instead of 64 bit int/float
using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

#endif
