#include "LoadJson.h"

#include <fstream>

namespace JsonHelper {

json openJsonFile(const std::string& file_location) {
  std::string filepath = file_location;
  auto frag_p = filepath.find('#');
  std::string frag;
  if (frag_p != std::string::npos) {
    frag = filepath.substr(frag_p + 1);
    filepath = filepath.substr(0, frag_p);
  }

  std::ifstream file(filepath);
  if (!file) {
    throw std::runtime_error("could not open file: " + filepath);
  }
  json j;
  try {
    j = json::parse(file);
  } catch (json::parse_error &e) {
    throw std::runtime_error(e.what());
  }
  file.close();
  // variant produces null for some parse errors
  if(j.is_null()) {
    throw std::runtime_error("Parsing json file produced null");
  }

  if(frag.empty()) {
    return j;
  } else {
    // Use `at` so it throws if the entry is not found
    return j.at(json::json_pointer(frag));
  }
}

} // Close namespace
