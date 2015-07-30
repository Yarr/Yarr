// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Config Base class
// ################################

#include "Fei4Cfg.h"

unsigned Fei4Cfg::getChipId() {
	return chipId;
}

void Fei4Cfg::setChipId(unsigned arg_chipId) {
	chipId = arg_chipId;
}

void Fei4Cfg::toFileBinary(std::string filename) {
    cfgName = filename;
    std::fstream outfile(filename.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
    if (outfile) {
        // General config
        outfile.write((char*)&chipId, sizeof(unsigned));
        outfile.write((char*)&sCap, sizeof(double));
        outfile.write((char*)&lCap, sizeof(double));
        outfile.write((char*)&vcalOffset, sizeof(double));
        outfile.write((char*)&vcalSlope, sizeof(double));

        // Global config
        outfile.write((char*)&cfg[0], sizeof(uint16_t)*numRegs);

        // Pixel config
        for (unsigned bit=0; bit<n_Bits; bit++) {
            for (unsigned dc=0; dc<n_DC; dc++) {
                outfile.write((char*)&getCfg(bit, dc)[0], sizeof(uint32_t)*n_Words);
            }
        }

    } else {
        std::cout << __PRETTY_FUNCTION__ << " --> ERROR: Could not open file: " << filename << std::endl;
    }
    outfile.close();
}

void Fei4Cfg::toFileBinary() {
    if (cfgName != "") {
        this->toFileBinary(cfgName);
    } else {
        std::cout << __PRETTY_FUNCTION__ << " --> ERROR: No filename specified!" << std::endl;
    }
}

void Fei4Cfg::fromFileBinary(std::string filename) {
    cfgName = filename;
    std::fstream infile(filename.c_str(), std::fstream::in | std::fstream::binary);
    if (infile) {
        // General config
        infile.read((char*)&chipId, sizeof(unsigned));
        infile.read((char*)&sCap, sizeof(double));
        infile.read((char*)&lCap, sizeof(double));
        infile.read((char*)&vcalOffset, sizeof(double));
        infile.read((char*)&vcalSlope, sizeof(double));

        // Global config
        infile.read((char*)&cfg[0], sizeof(uint16_t)*numRegs);

        // Pixel config
        for (unsigned bit=0; bit<n_Bits; bit++) {
            for (unsigned dc=0; dc<n_DC; dc++) {
                infile.read((char*)&getCfg(bit, dc)[0], sizeof(uint32_t)*n_Words);
            }
        }

    } else {
        std::cout << __PRETTY_FUNCTION__ << " --> ERROR: Could not open file: " << filename << std::endl;
    }
    infile.close();

}

void Fei4Cfg::fromFileBinary() {
    if (cfgName != "") {
        this->fromFileBinary(cfgName);
    } else {
        std::cout << __PRETTY_FUNCTION__ << " --> ERROR: No filename specified!" << std::endl;
    }
}
