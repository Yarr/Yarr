#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iomanip>
#include <sstream>

namespace Utils {
    static inline std::string hexify(int number) {
        std::stringbuf buf;
        std::ostream os(&buf);

        os << "0x" << std::setfill('0') << std::hex << number;

        return buf.str();
    }
}
#endif
