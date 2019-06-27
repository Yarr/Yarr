#include "ScanHelper.h"

#include <iostream>
#include <fstream>
#include <exception>

namespace ScanHelper {
    json openJsonFile(std::string filepath) {
        std::ifstream file(filepath);
        if (!file) {
            throw std::runtime_error("could not open file");
        }
        json j;
        try {
            j = json::parse(file);
        } catch (json::parse_error &e) {
            throw std::runtime_error(e.what());
        }
        return j;
    }
}
