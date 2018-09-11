#ifndef SPECINFO_H__
#define SPECINFO_H__

#include "SpecCom.h"
#include <cstdint>

#define BOARD_INFO_BASE		(0xF << 14)
#define BOARD_INFO_DATE		0
#define BOARD_INFO_ID		1
#define BOARD_INFO_UPTIME	2
#define BOARD_INFO_HASH0	3
#define BOARD_INFO_HASH1	4
#define BOARD_INFO_HASH2	5
#define BOARD_INFO_HASH3	6
#define BOARD_INFO_HASH4	7

class SpecInfo {
public:
	enum BaseBoard {
		BaseBoardSpec         = 0x0,
		BaseBoardKC705        = 0x1,
		BaseBoardXpressK7_160 = 0x2,
		BaseBoardXpressK7_325 = 0x3
	};

	enum FMCBoard {
		FMCBoardQuadFEI4      = 0x0,
		FMCBoardOctaFEI4      = 0x1,
		FMCBoardLBNLRD53      = 0x2,
		FMCBoardOhioRD53      = 0x3,
		FMCBoardFE65P2        = 0x4,
		FMCBoardQSFPx2        = 0x5
	};

	enum FEType {
		FETypeFEI4            = 0x0,
		FETypeRD53            = 0x1
	};

	// constructor
	SpecInfo(SpecCom *arg_spec);

	// git info
	std::string getCommitHash();
	uint32_t getCommitDate();
	std::string getCommitDateString();

	// uptime
	uint32_t getUptime();

	// board ID
	uint32_t getBoardId();
	BaseBoard getBaseBoard();
	FMCBoard getFMCBoard();
	FEType getFEType();
	std::string getBoardString();

	// channel numbers
	int getNumTx();
	int getNumRx();

private:
	SpecCom *m_spec;
};

#endif // SPECINFO_H__
