#ifndef _STORAGE_HPP_
#define  _STORAGE_HPP_
#if defined(USE_JSON)
#include <json.hpp>
using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;
#else
#include "variant.hpp"
using json=variant32;
#endif
#endif
