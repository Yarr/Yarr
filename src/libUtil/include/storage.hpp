#ifndef _STORAGE_HPP_
#define  _STORAGE_HPP_
#include <json.hpp>
using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;
#endif
