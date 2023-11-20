#include <iostream>
#include <chrono>
#include <unistd.h>
#include <bitset>
#include <iomanip>
#include <random>
#include <queue>
#include <stack>

#include "storage.hpp"
#include "logging.h"
#include "LoggingConfig.h"

#include "ScanHelper.h"
#include "Bookkeeper.h"

#include "Rd53b.h"
#include "Rd53bPixelCfg.h"

auto logger = logging::make_log("rd53bRegSEETest");

std::unique_ptr<std::ofstream> badpix;
std::unique_ptr<std::ofstream> badglob;
std::unique_ptr<std::ofstream> rawdata;

std::vector<RawData> processDataPackets(HwController *hwCtrl)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::vector<RawData> dataList;
    std::vector<RawDataPtr> dataVec;
    RawDataPtr data;
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        dataVec = hwCtrl->readData();
        if (dataVec.size() > 0)
        {
            dataList.push_back(*dataVec[0]);
        }
    } while (dataVec.size() > 0);

    return dataList;
}

std::vector<uint16_t> readData(RawData *data)
{
    std::vector<uint16_t> result;
    int frameNum = 0;
    unsigned zz = 0;
    unsigned stat = 0;
    unsigned addr1 = 0;
    unsigned val1 = 0;
    unsigned val1_1 = 0;
    unsigned val1_2 = 0;
    unsigned addr2 = 0;
    unsigned val2 = 0;

    for (unsigned i = 0; i < data->getSize(); i++)
    {
        if (data->get(i) != 0xFFFFFFFF)
        {
            *rawdata << data->get(i) << std::endl;
            if (frameNum % 2 == 0)
            {
                zz = 0;
                stat = 0;
                addr1 = 0;
                val1 = 0;
                val1_1 = 0;
                val1_2 = 0;
                addr2 = 0;
                val2 = 0;

                zz = 0xFF & (data->get(i) >> 24);

                if (!(zz == 0x55 || zz == 0x99))
                {
                    logger->warn("wrong Aurora code: [zz] = 0x{:x}. Skip current frame", zz);
                    i++;
                    frameNum += 2;
                    continue;
                }

                val1_1 = (0x3FF & (data->get(i) >> 0)) << 6;
            }
            else
            {

                val1_2 = 0x3F & (data->get(i) >> 26);
                val1 = val1_1 + val1_2;

                val2 = 0xFFFF & (data->get(i) >> 0);

                const uint16_t val = (zz == 0x99) ? val1 : val2;
                result.push_back(val);
            }
        }
        frameNum++;
    }
    // logger->info("There are {} frames read in total", frameNum);

    return result;
}

/* Decode pixel registers */
void decodePix(std::vector<RawData> dataList, Rd53b &rd53b, std::vector<std::pair<uint16_t, uint16_t>> &address)
{
    std::vector<uint16_t> result;
    for (auto data : dataList)
    {
        std::vector<uint16_t> tmp = readData(&data);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }

    for (unsigned i = 0; i < result.size();)
    {
        if (i + 2 >= result.size())
        {
            logger->warn("Incomplete data for pixel register reading. Will skip the last unreadable block");
            break;
        }
        uint16_t dc = result[i + 1];
        uint16_t row = result[i + 2];
        uint16_t val = result[i];

        auto it = std::find(address.begin(), address.end(), std::make_pair(dc, row));
        if (it != address.end())
        {
            address.erase(it);
        }
        else
        {
            logger->warn("Pixel address dc {}, row {} does not match any address of pixels processed.", dc, row);
            // *badpix << dc << " " << row << std::endl;
            /* What usually happens is the pix portal is read into dc or row. This can be checked by asking whether the number is out of range */
            if (dc >= Rd53b::n_DC){
                logger->warn("DC number is invalid. Increase index by 1: {}", i);
                i = i + 1;
            }
            else if (row >= Rd53b::n_Row){
                logger->warn("Row number is invalid. Increase index by 2: {}", i);
                i = i + 2; 
            }
            else{
                i = i + 3;
            }
            continue;
        }
        // std::cout << val1 << " " << val2 << std::endl;
        // std::cout << dc << " " << row << " " << pix1.s.en << " " << pix1.s.hitbus << " " << pix1.s.injen << " " << pix1.s.tdac * (pix1.s.sign == 0 ? +1 : -1) << std::endl;
        // std::cout << dc << " " << row << " " << pix2.s.en << " " << pix2.s.hitbus << " " << pix2.s.injen << " " << pix2.s.tdac * (pix2.s.sign == 0 ? +1 : -1) << std::endl;
        // getchar();
        Rd53bPixelCfg::pixelBits pix1, pix2;
        pix1.u8 = (val & 0xFF);
        pix2.u8 = (val >> 8) & 0xFF;
        rd53b.setReg(dc * 2, row, pix1.s.en, pix1.s.injen, pix1.s.hitbus, pix1.s.tdac * (pix1.s.sign == 0 ? +1 : -1));
        rd53b.setReg(dc * 2 + 1, row, pix2.s.en, pix2.s.injen, pix2.s.hitbus, pix2.s.tdac * (pix2.s.sign == 0 ? +1 : -1));

        i += 3;
    }
}

/* Decode global registers, one-by-one */
uint16_t decodeGlob(std::vector<RawData> dataList, int address)
{
    std::vector<uint16_t> result;
    for (auto data : dataList)
    {
        std::vector<uint16_t> tmp = readData(&data);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    if (result.size() > 1){
        logger->warn("There are more than one set of readback for a single global register! Will use the first value");
        *badglob << address << " " << result.size() << std::endl;
    }
    else if (result.size() == 0)
    {
        *badglob << address << " " << result.size() << std::endl;
        logger->warn("There are no data readback! Will use value 0");
        return 0;
    }
    return result[0];
}

void saveCfgFile(Rd53b &rd53b, std::string cfgFilePath, json globSEU)
{
    std::ofstream newCfgFile(cfgFilePath.c_str());
    json cfg;
    rd53b.writeConfig(cfg);
    cfg["GlobalSEU"] = globSEU;
    newCfgFile << std::setw(4) << cfg;
    newCfgFile.close();
    logger->info("Output file {} saved.", cfgFilePath.c_str());
}

int main(int argc, char *argv[])
{
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);

    logger->info("\033[1;31m#################################\033[0m");
    logger->info("\033[1;31m# RD53B Test Tool: SEE register #\033[0m");
    logger->info("\033[1;31m#################################\033[0m");

    logger->info("Parsing command line parameters ...");
    int c;
    std::string cfgFilePath = "configs/rd53b_test.json";
    std::string ctrlFilePath = "configs/controller/specCfg-rd53b.json";
    bool randomize = false, initialize = false;
    int seed = 0, waitTime = 300;
    std::string outputPrefix = "RD53B_SEE";
    const unsigned nRead = 2; /* Read back two times */
    const unsigned counter_max = Rd53b::n_Row;

    while ((c = getopt(argc, argv, "c:r:m:s:o:i:w:")) != -1)
    {
        int count = 0;
        switch (c)
        {
        case 'r':
            ctrlFilePath = std::string(optarg);
            break;
        case 'c':
            cfgFilePath = std::string(optarg);
            break;
        case 'm':
            randomize = std::atoi(optarg);
            break;
        case 's':
            seed = std::atoi(optarg);
            break;
        case 'o':
            outputPrefix = std::string(optarg);
            break;
        case 'i':
            initialize = std::atoi(optarg);
            break;
        case 'w':
            waitTime = std::atoi(optarg);
            break;            
        default:
            spdlog::critical("No command line parameters given!");
            return -1;
        }
    }

    logger->info("Chip config file path  : {}", cfgFilePath);
    logger->info("Ctrl config file path  : {}", ctrlFilePath);

    logger->info("\033[1;31m#################\033[0m");
    logger->info("\033[1;31m# Init Hardware #\033[0m");
    logger->info("\033[1;31m#################\033[0m");

    logger->info("-> Opening controller config: {}", ctrlFilePath);

    std::unique_ptr<HwController> hwCtrl = nullptr;
    json ctrlCfg;
    try
    {
        ctrlCfg = ScanHelper::openJsonFile(ctrlFilePath);
        hwCtrl = ScanHelper::loadController(ctrlCfg);
    }
    catch (std::runtime_error &e)
    {
        logger->critical("Error opening or loading controller config: {}", e.what());
        return -1;
    }

    hwCtrl->runMode();
    hwCtrl->setTrigEnable(0);
    hwCtrl->disableRx();

    Bookkeeper bookie(&*hwCtrl, &*hwCtrl);

    logger->info("\033[1;31m###################\033[0m");
    logger->info("\033[1;31m##  Chip Config  ##\033[0m");
    logger->info("\033[1;31m###################\033[0m");

    Rd53b rd53b;
    rd53b.init(&*hwCtrl, FrontEndConnectivity(0,0));

    std::ifstream cfgFile(cfgFilePath);
    if (cfgFile)
    {
        // Load config
        logger->info("Loading config file: {}", cfgFilePath);
        json cfg;
        try
        {
            cfg = ScanHelper::openJsonFile(cfgFilePath);
        }
        catch (std::runtime_error &e)
        {
            logger->error("Error opening chip config: {}", e.what());
            throw(std::runtime_error("loadChips failure"));
        }
        rd53b.loadConfig(cfg);
        cfgFile.close();
    }
    else
    {
        logger->warn("Config file not found, using default!");
        // Write default to file
        std::ofstream newCfgFile(cfgFilePath.c_str());
        json cfg;
        rd53b.writeConfig(cfg);
        newCfgFile << std::setw(4) << cfg;
        newCfgFile.close();
    }
    /* Creating output directory */
    if (outputPrefix.find('/') != std::string::npos)
    {
        std::string outputDirName = outputPrefix.substr(0, outputPrefix.find_last_of('/'));
        system((std::string("mkdir -vp ") + outputDirName).c_str());
    }

    hwCtrl->setCmdEnable(0);
    while (!hwCtrl->isCmdEmpty())
        ;
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    if (initialize)
    {
        rd53b.configureInit();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        while (!hwCtrl->isCmdEmpty());

        rd53b.configureGlobal();
        /* Put global threshold to high value to avoid large current */
        rd53b.writeRegister(&Rd53b::DiffTh1L, 400);
        rd53b.writeRegister(&Rd53b::DiffTh1R, 400);
        rd53b.writeRegister(&Rd53b::DiffTh1M, 400);
        rd53b.writeRegister(&Rd53b::DiffVff, 200);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        while (!hwCtrl->isCmdEmpty());

        if (randomize)
        {
            /* Randomize pixel registers */
            std::mt19937 generator;
            generator.seed(seed);
            std::uniform_int_distribution<> tdac(-15, 15);
            std::uniform_int_distribution<> mask(0, 1);
            for (unsigned col = 0; col < Rd53b::n_Col; col++)
            {
                for (unsigned row = 0; row < Rd53b::n_Row; row++)
                {
                    rd53b.setEn(col, row, mask(generator));
                    rd53b.setInjEn(col, row, mask(generator));
                    rd53b.setHitbus(col, row, mask(generator));
                    rd53b.setTDAC(col, row, tdac(generator));
                }
            }

            /* Randomize global registers for SEU: 138--255, each 16 bits */
            /* These registers are not implemented, so need to save them in a separate file */
            json j_glob_seu; // empty
            for (uint16_t addr = 138; addr < 256; addr++)
            {
                std::string regName = addr < 202 ? ("SEU" + std::to_string(addr - 138) + "_notmr") : ("SEU" + std::to_string(addr - 202));
                std::uniform_int_distribution<> regVal(0, 0xFFFF);
                j_glob_seu[regName] = regVal(generator);
                rd53b.sendWrReg(rd53b.getChipId(), addr, j_glob_seu[regName]);
                while (!hwCtrl->isCmdEmpty())
                    ;
            }

            /* Save the config file */
            saveCfgFile(rd53b, outputPrefix + "_rndm" + std::to_string(seed) + ".json", j_glob_seu);
        }
        rd53b.configurePixels();
        while (!hwCtrl->isCmdEmpty())
            ;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    hwCtrl->setRxEnable(0);
    while (!hwCtrl->isCmdEmpty())
        ;
    hwCtrl->flushBuffer();

    /* Read back pixel registers */
    Rd53b rd53b_readback[2][nRead];
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    for (unsigned stage = 0; stage < 2; stage++)
    {
        std::string outputPrefix_stage = outputPrefix + (stage == 0 ? "_input" : "_readback");
        for (unsigned idx = 0; idx < nRead; idx++)
        {
            /* Start files documenting bad pixels and raw data */
            rawdata.reset(new std::ofstream(outputPrefix_stage + "_rawdata" + std::to_string(idx) + ".txt", std::ofstream::binary));
            badpix.reset(new std::ofstream(outputPrefix_stage + "_badpix" + std::to_string(idx) + ".txt"));
            badglob.reset(new std::ofstream(outputPrefix_stage + "_badglob" + std::to_string(idx) + ".txt"));
            /* Read back pixel registers */
            unsigned counter = 0;
            std::vector<std::pair<uint16_t, uint16_t>> address;

            for (unsigned dc = 0; dc < Rd53b::n_DC; dc++)
            {
                // logger->info("DC {}", dc);
                for (unsigned row = 0; row < Rd53b::n_Row; row++)
                {
                    // logger->info("Row {}", row);
                    rd53b.writeRegister(&Rd53b::PixRegionCol, dc);
                    rd53b.writeRegister(&Rd53b::PixRegionRow, row);
                    rd53b.readRegister(&Rd53b::PixPortal);
                    while (!hwCtrl->isCmdEmpty())
                        ;
                    rd53b.readRegister(&Rd53b::PixRegionCol);
                    while (!hwCtrl->isCmdEmpty())
                        ;
                    rd53b.readRegister(&Rd53b::PixRegionRow);
                    while (!hwCtrl->isCmdEmpty())
                        ;

                    address.push_back(std::make_pair(dc, row));
                    counter++;
                    if (counter >= counter_max)
                    {
                        std::vector<RawData> dataList = processDataPackets(hwCtrl.get());

                        decodePix(dataList, rd53b_readback[stage][idx], address);
                        if (!address.empty())
                        {
                            logger->warn("Not all pixels are processed!");
                            for (auto item : address)
                            {
                                *badpix << item.first << " " << item.second << std::endl;
                            }
                            address.clear();
                        }
                        counter = 0;
                    }
                }
            }

            /* Read back global registers, starting from normal ones */
            for (unsigned addr = 0; addr < 138; addr++)
            {
                rd53b.sendRdReg(rd53b.getChipId(), addr);
                while (!hwCtrl->isCmdEmpty())
                    ;
                std::vector<RawData> dataList = processDataPackets(hwCtrl.get());
                rd53b_readback[stage][idx][addr] = decodeGlob(dataList, addr);
            }

            json j_glob_seu; // empty
            for (uint16_t addr = 138; addr < 256; addr++)
            {
                std::string regName = addr < 202 ? ("SEU" + std::to_string(addr - 138) + "_notmr") : ("SEU" + std::to_string(addr - 202));
                rd53b.sendRdReg(rd53b.getChipId(), addr);
                while (!hwCtrl->isCmdEmpty())
                    ;
                std::vector<RawData> dataList = processDataPackets(hwCtrl.get());
                j_glob_seu[regName] = decodeGlob(dataList, addr);
            }

            /* Use config file to record readback values. In the end will analyze the config files for SEE effects */
            if (stage == 0 && idx == 0)
                start = std::chrono::steady_clock::now();
            else if (stage == 1 && idx == nRead - 1)
                end = std::chrono::steady_clock::now();
            saveCfgFile(rd53b_readback[stage][idx], outputPrefix_stage + "_rb" + std::to_string(idx) + ".json", j_glob_seu);
            badpix->close();
            badglob->close();
            rawdata->close();
        }
        if (stage == 0)
            std::this_thread::sleep_for(std::chrono::seconds(waitTime));
    }
    /* Put pixel readout registers to largest unphysical values */
    rd53b.writeRegister(&Rd53b::PixPortal, 0xffff); /* 16 bit */
    rd53b.writeRegister(&Rd53b::PixRegionCol, 0xffff); /* 16 bit */
    rd53b.writeRegister(&Rd53b::PixRegionRow, 0xffff); /* 16 bit */
    rd53b.writeRegister(&Rd53b::PixAutoRow, 0x0); /* 1 bit */
    rd53b.writeRegister(&Rd53b::AiRegionRow, 0x1ff); /* 9 bit */
    while (!hwCtrl->isCmdEmpty())
        ;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::ofstream ftime(outputPrefix + "_time.txt");
    std::chrono::duration<double> diff = end - start;
    ftime << diff.count() << std::endl;
    ftime.close();

    hwCtrl->disableRx();
    return 0;
}
