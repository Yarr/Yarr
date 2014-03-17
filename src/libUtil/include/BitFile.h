#ifndef BITFILE_H
#define BITFILE_H

#include <fstream>

namespace BitFile{
    int checkFile(std::fstream &file);
    size_t getSize(std::fstream &file);
}

#endif
