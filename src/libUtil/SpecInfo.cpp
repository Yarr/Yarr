#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "SpecInfo.h"

SpecInfo::SpecInfo(SpecCom *arg_spec)
{
	m_spec = arg_spec;
}

std::string SpecInfo::getCommitHash()
{
	std::stringstream ss;

	for(int i = 0; i < 5; i++)
	{
		ss << std::hex << std::setw(8) << std::setfill('0') << m_spec->readSingle(BOARD_INFO_BASE + BOARD_INFO_HASH0 + i) << std::setfill(' ') << std::setw(0) << std::dec;
	}

	return ss.str();
}

uint32_t SpecInfo::getCommitDate()
{
	return m_spec->readSingle(BOARD_INFO_BASE + BOARD_INFO_DATE);
}

std::string SpecInfo::getCommitDateString()
{
	char buf[128];
	time_t time = (time_t)getCommitDate();
	struct tm * timeinfo;
	timeinfo = localtime(&time);
	strftime(buf, 128, "%c", timeinfo);

	std::string ret(buf);
	return ret;
}

uint32_t SpecInfo::getUptime()
{
	return m_spec->readSingle(BOARD_INFO_BASE + BOARD_INFO_UPTIME);
}

uint32_t SpecInfo::getBoardId()
{
	return m_spec->readSingle(BOARD_INFO_BASE + BOARD_INFO_ID);
}

SpecInfo::BaseBoard SpecInfo::getBaseBoard()
{
	uint32_t id = getBoardId();

	return (SpecInfo::BaseBoard)((id >> 28) & 0xF);
}

SpecInfo::FMCBoard SpecInfo::getFMCBoard()
{
	uint32_t id = getBoardId();

	return (SpecInfo::FMCBoard)((id >> 23) & 0x1F);
}

SpecInfo::FEType SpecInfo::getFEType()
{
	uint32_t id = getBoardId();

	return (SpecInfo::FEType)((id >> 20) & 0x7);
}

std::string SpecInfo::getBoardString()
{
	std::stringstream ss;
	BaseBoard base = getBaseBoard();
	FMCBoard fmc = getFMCBoard();
	FEType fe = getFEType();

	ss << "Baseboard: ";
	switch(base)
	{
		case BaseBoardSpec: ss << "SPEC"; break;
		case BaseBoardKC705: ss << "KC705"; break;
		case BaseBoardXpressK7_160: ss << "XpressK7-160"; break;
		case BaseBoardXpressK7_325: ss << "XpressK7-325"; break;
		default: ss << "unknown"; break;
	}

	ss << ", FMC: ";
	switch(fmc)
	{
		case FMCBoardQuadFEI4: ss << "Quad-FE-I4"; break;
		case FMCBoardOctaFEI4: ss << "Octa-FE-I4"; break;
		case FMCBoardOhioRD53: ss << "Ohio-RD53"; break;
		case FMCBoardLBNLRD53: ss << "LBNL-RD53"; break;
		case FMCBoardFE65P2: ss << "FE65P2"; break;
		case FMCBoardQSFPx2: ss << "HTG-FMC-QSFP28-x2"; break;
		default: ss << "unknown"; break;
	}

	ss << ", FE: ";
	switch(fe)
	{
		case FETypeFEI4: ss << "FE-I4"; break;
		case FETypeRD53: ss << "RD53A"; break;
		default: ss << "unknown"; break;
	}

	return ss.str();
}

int SpecInfo::getNumTx()
{
	uint32_t id = getBoardId();
	return (id & 0xFF);
}

int SpecInfo::getNumRx()
{
	uint32_t id = getBoardId();
	return ((id >> 8) & 0xFF);
}
