#ifndef STAR_CFG_INCLUDE
#define STAR_CFG_INCLUDE

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: Star configuration class
// ################################

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include "BitOps.h"

#include "FrontEnd.h"




class SubRegister{

public:

	SubRegister(){
		m_parentReg			= 0;
		m_parentRegAddress	= -1;
		m_subRegName			= "";
		m_bOffset 			= 0;
		m_mask				= 0;
		m_msbRight 			= false;
		m_value 				= 0;
	};


	SubRegister(uint32_t *reg=NULL, int parentRegAddress=-1, std::string subRegName="", unsigned bOffset=0, unsigned mask=0, bool msbRight=false){
		m_parentReg			= reg;
		m_parentRegAddress	= parentRegAddress;
		m_subRegName			= subRegName;
		m_bOffset 			= bOffset;
		m_mask				= mask;
		m_msbRight			= msbRight;

	};


	// Get value of field
	const unsigned getValue() {
		unsigned maskBits = (1<<m_mask)-1;
		unsigned tmp = ((*m_parentReg&(maskBits<<m_bOffset))>>m_bOffset);

		if(m_msbRight?BitOps::reverse_bits(tmp, m_mask):tmp != m_value){
			std::cerr << " --> Error: Stored value and retrieve value does not match: \""<< m_subRegName << "\"" << std::endl;
		}

		return (m_msbRight?BitOps::reverse_bits(tmp, m_mask):tmp);
	}



	// Write value to field and config
	void updateValue(const uint32_t cfgBits) {
		m_value = cfgBits;

		unsigned maskBits = (1<<m_mask)-1;
		*m_parentReg=(*m_parentReg&(~(maskBits<<m_bOffset))) |
				(((m_msbRight?BitOps::reverse_bits(cfgBits, m_mask):cfgBits)&maskBits)<<m_bOffset);
	}


	std::string name() const{ return m_subRegName; }


	int getParentRegAddress() const{ return m_parentRegAddress; }

	uint32_t getParentRegValue() const{ return *m_parentReg;	}



private:
	uint32_t *m_parentReg;
	int m_parentRegAddress;
	std::string m_subRegName;
	unsigned m_bOffset;
	unsigned m_mask;
	bool m_msbRight;
	unsigned m_value;



};



class Register{

public:

	std::map<std::string, SubRegister*> subRegisterMap;

	Register(int addr=-1, uint32_t value=0){
		m_regAddress		= addr;
		m_regValue			= value;
	};


	~Register(){
		std::map<std::string, SubRegister*>::iterator map_iter;
		for(map_iter=subRegisterMap.begin(); map_iter!= subRegisterMap.end(); ++map_iter){
			delete map_iter->second;
		}
		subRegisterMap.clear();
	};




	int addr() const{ return m_regAddress;}
	const uint32_t getValue() const{ return m_regValue;}
	void setValue(uint32_t value) {m_regValue	= value;}
	void setMySubRegisterValue(std::string subRegName, uint32_t value){
		subRegisterMap[subRegName]->updateValue(value);
	}

	const unsigned getMySubRegisterValue(std::string subRegName){
		return subRegisterMap[subRegName]->getValue();
	}


	SubRegister* addSubRegister(std::string subRegName="", unsigned bOffset=0, unsigned mask=0, bool msbRight=false){
		subRegisterMap[subRegName] = new SubRegister(&m_regValue, m_regAddress, subRegName,bOffset, mask, msbRight);
		return subRegisterMap[subRegName];
	}


private:
	int m_regAddress;
	uint32_t m_regValue;



};




class StarCfg : public FrontEndCfg, public Register{
public:
    StarCfg();
    ~StarCfg();

    //This saves all Register objects to memory, purely for storage.  We will never access registers from this
    std::vector< Register > AllReg_List;
    //This is a 2D map of each register to the chipID and address.  For example registerMap[chipID][addr]
    std::map<unsigned, std::map<unsigned, Register*> >registerMap; //Maps register address
    //This is a 2D map of each subregister to the chipID and subregister name.  For example subRegisterMap_all[chipID][NAME]
    std::map<unsigned, std::map<std::string, SubRegister*> > subRegisterMap_all;   //register record

    //Function to make all Registers for the HCC and ABC
    void configure_HCC_Registers();
    void configure_ABC_Registers(int chipID);

    //Initialized the registers of the HCC and ABC.  Do afer JSON file is loaded.
    void initRegisterMaps();

    unsigned getChipId(){return m_hccID;}
    void setChipId(unsigned hccID){
    	m_hccID = hccID;
    }




	void setSubRegisterValue(int iChip, std::string subRegName, uint32_t value) {
		if (subRegisterMap_all[iChip].find(subRegName) != subRegisterMap_all[iChip].end()) {
			subRegisterMap_all[iChip][subRegName]->updateValue(value);
		} else {
			std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
		}
	}


	uint32_t getSubRegisterValue(int iChip, std::string subRegName) {
		if (subRegisterMap_all[iChip].find(subRegName) != subRegisterMap_all[iChip].end()) {

			return subRegisterMap_all[iChip][subRegName]->getValue();
		} else {
			std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
		}
		return 0;
	}


	int getSubRegisterParentAddr(int iChip, std::string subRegName) {
		if (subRegisterMap_all[iChip].find(subRegName) != subRegisterMap_all[iChip].end()) {
			return subRegisterMap_all[iChip][subRegName]->getParentRegAddress();
		} else {
			std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
		}
		return 0;
	}


	uint32_t getSubRegisterParentValue(int iChip, std::string subRegName) {
		if (subRegisterMap_all[iChip].find(subRegName) != subRegisterMap_all[iChip].end()) {
			return subRegisterMap_all[iChip][subRegName]->getParentRegValue();
		} else {
			std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
		}
		return 0;
	}



    // Don't use for now
    double toCharge(double) override { return 0.0; };
    double toCharge(double, bool, bool) override { return 0.0; };

    void toFileJson(json &j) override;
    void fromFileJson(json &j) override;

    void toFileBinary() override;
    void fromFileBinary() override;


protected:
	unsigned m_hccID;
	int m_nABC = 0;
	std::vector<int> m_chipIDs;
};

#endif
