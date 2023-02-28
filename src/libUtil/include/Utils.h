#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iomanip>
#include <sstream>

namespace Utils {
    static inline std::string hexify(const int &number) {
        std::stringbuf buf;
        std::ostream os(&buf);

        os << "0x" << std::setfill('0') << std::hex << number;

        return buf.str();
    }

    static inline std::string dirFromPath(const std::string &path) {
        std::string directory;
        const size_t last_slash_idx = path.rfind('/');
        if (std::string::npos != last_slash_idx)
        {
                directory = path.substr(0, last_slash_idx);
        }
        return directory;
    }
}
#endif
