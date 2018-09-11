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

void Fei4Cfg::toFileXml(tinyxml2::XMLDocument *doc) {
    tinyxml2::XMLElement *fe = doc->NewElement("FE-I4B");
    
    tinyxml2::XMLElement *par = doc->NewElement("Parameter");
    
    tinyxml2::XMLElement *ele = doc->NewElement("Name");
    ele->SetText(name.c_str());
    ele->SetAttribute("type", "string");
    par->LinkEndChild(ele);

    ele = doc->NewElement("chipId");
    ele->SetText(chipId);
    ele->SetAttribute("type", "dec");
    par->LinkEndChild(ele);

    ele = doc->NewElement("sCap");
    ele->SetText(sCap);
    ele->SetAttribute("type", "double");
    ele->SetAttribute("unit", "fF");
    par->LinkEndChild(ele);

    ele = doc->NewElement("lCap");
    ele->SetText(lCap);
    ele->SetAttribute("type", "double");
    ele->SetAttribute("unit", "fF");
    par->LinkEndChild(ele);

    ele = doc->NewElement("vcalOffset");
    ele->SetText(vcalOffset);
    ele->SetAttribute("type", "double");
    ele->SetAttribute("unit", "mV");
    par->LinkEndChild(ele);

    ele = doc->NewElement("vcalSlope");
    ele->SetText(vcalSlope);
    ele->SetAttribute("type", "double");
    ele->SetAttribute("unit", "mV");
    par->LinkEndChild(ele);

    fe->LinkEndChild(par);
    Fei4GlobalCfg::toFileXml(doc, fe);
    Fei4PixelCfg::toFileXml(doc, fe);
    doc->LinkEndChild(fe);
}

void Fei4Cfg::toFileJson(json &j) {
    j["FE-I4B"]["name"] = name;

    j["FE-I4B"]["Parameter"]["chipId"] = chipId;
    j["FE-I4B"]["Parameter"]["sCap"] = sCap;
    j["FE-I4B"]["Parameter"]["lCap"] = lCap;
    j["FE-I4B"]["Parameter"]["vcalOffset"] = vcalOffset;
    j["FE-I4B"]["Parameter"]["vcalSlope"] = vcalSlope;

    Fei4PixelCfg::toFileJson(j);
    Fei4GlobalCfg::toFileJson(j);
}

void Fei4Cfg::fromFileJson(json &j) {
    if (!j["FE-I4B"]["name"].empty())
        name = j["FE-I4B"]["name"];

    if (!j["FE-I4B"]["Parameter"]["chipId"].empty())
        chipId = j["FE-I4B"]["Parameter"]["chipId"];
    if (!j["FE-I4B"]["Parameter"]["sCap"].empty())
        sCap = j["FE-I4B"]["Parameter"]["sCap"];
    if (!j["FE-I4B"]["Parameter"]["lCap"].empty())
        lCap = j["FE-I4B"]["Parameter"]["lCap"];
    if (!j["FE-I4B"]["Parameter"]["vcalOffset"].empty())
        vcalOffset = j["FE-I4B"]["Parameter"]["vcalOffset"];
    if (!j["FE-I4B"]["Parameter"]["vcalSlope"].empty())
        vcalSlope = j["FE-I4B"]["Parameter"]["vcalSlope"];

    Fei4PixelCfg::fromFileJson(j);
    Fei4GlobalCfg::fromFileJson(j);
}
