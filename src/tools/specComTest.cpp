#include <stdint.h>
#include <string.h>

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"

auto logger = logging::make_log("specComTest");

int main(int argc, char **argv) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
    
    // Init spec
    logger->info("Init spec");
    int specNum = 0;
    if (argc == 2)
        specNum = atoi(argv[1]);
    SpecCom mySpec(specNum);
    std::string tmp;
    const size_t size = 256*8;
    unsigned err_count = 0;
    
    uint32_t *data = new uint32_t[size];
    for(unsigned i=0; i<size;i++)
        data[i] = i;

    uint32_t *resp = new uint32_t[size];

    logger->info("Starting DMA write/read test ...");
    memset(resp, size*4, 0x5A);
    
    mySpec.writeDma(0x0, data, size); 
    logger->info("... writing {} byte.", size*4);
    mySpec.readDma(0x0, resp, size); 
    logger->info("... read {} byte.", size*4);
    
    for (unsigned i=0; i<size; i++) {
        if (data[i] != resp[i]) {
            logger->error("[{}] {:x} \t {:x}", i, data[i], resp[i]);
            err_count++;
        }
    }

    if (err_count == 0) {
        logger->info("Success! No errors.");
    } else {
        logger->critical("DMA transmission failed!");
    }
    
    delete[] data;
    delete[] resp;

    return 0;
}
