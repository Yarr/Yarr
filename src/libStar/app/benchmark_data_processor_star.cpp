#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include "AllProcessors.h"
#include "EventData.h"
#include "StarCfg.h"

#include "logging.h"
#include "LoggingConfig.h"

auto logger = logging::make_log("benchmark_dataprocessing_star");

void run_test(StarCfg &cfg, FeDataProcessor &proc, int iterations, std::vector<uint8_t> &buffer);

std::vector<uint8_t> read_file(const char *file_name) {
    std::vector<uint8_t> buffer;

    std::error_code ec;
    std::filesystem::path name(file_name);
    auto len = std::filesystem::file_size(name, ec);
    if(ec) return {};
    buffer = std::vector<uint8_t>(len);
    std::ifstream in(name, std::ios::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), len);
    return buffer;
}

void printHelp() {
  std::cout << "Run data process benchmark (star version)\n";
  std::cout << "  benchmark_data_processor data_file [iters]\n";
}

int main(int argc, char *argv[]) {
    json loggerConfig;
    loggerConfig["pattern"] = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    loggerConfig["log_config"][0]["name"] = "all";
    loggerConfig["log_config"][0]["level"] = "info";
    loggerConfig["outputDir"] = "";
    logging::setupLoggers(loggerConfig);

    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("Star");
    if(argc == 1) {
        printHelp();
        return 0;
    }

    unsigned iterations = 100;

    if(argc == 3) {
        iterations = atoi(argv[2]);
    }

    auto buffer = read_file(argv[1]);
    if(buffer.empty()) {
      std::cout << "Aborting, buffer empty\n";
      return 1;
    }

    StarCfg cfg(1, 1);
    cfg.setHCCRegister(40, 0x7ff);

    run_test(cfg, *proc, iterations, buffer);

    return 0;
}

void run_test(StarCfg &cfg, FeDataProcessor &proc, int iterations, std::vector<uint8_t> &buffer) {

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    proc.connect(&cfg, &rd_cp, &em_cp);

    proc.init();

    std::thread proc_thread([&proc]() { proc.process(); });

    std::size_t nbits{};
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < iterations; i++) {
        uint32_t index = 0;
        uint32_t n = *((uint32_t *) &buffer[index]);
        index += sizeof(uint32_t);
        // logger->info("Loading {} packets", n);
        for (unsigned k = 0; k < n; k++) {
            struct Data {
                uint64_t timestamp;
                uint64_t id;
                uint32_t nbytes;
            } __attribute__ ((packed));
            if(index + sizeof(Data) > buffer.size()) {
                logger->error("Failed to read data for index {}", k);
                break;
            }

            Data &data = *(Data *)&buffer[index];
            index += sizeof(Data);
            // auto ts = data.timestamp;
            auto eid = data.id;
            auto nb = data.nbytes;
            // logger->info("Loading packet {} {:x} {} {}", index, ts, eid, nb);

            std::vector<uint32_t> edata((uint32_t *) &buffer[index], ((uint32_t *) &buffer[index]) + (nb+3)/4);
            nbits += nb*8;
            index += sizeof(uint8_t) * nb;
            RawDataPtr rd = std::make_shared<RawData>(eid, std::move(edata));
            std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus({1}, {LOOP_STYLE_MASK})));

            rdc->add(std::move(rd));
            rd_cp.pushData(std::move(rdc));
        }
    }
    auto packets = rd_cp.getNumDataIn();
    logger->info("Packets pushed {}: ", packets);
    std::size_t last = 0;
    auto last_log = std::chrono::steady_clock::now();
    while (true) {
        std::size_t sz= em_cp.size();
        auto log_check = std::chrono::steady_clock::now();
        if((log_check - last_log) > std::chrono::milliseconds(100)) {
            logger->info("Progress {}%", sz*100/packets);
            last_log = log_check;

            // Timeout check
            if(last == sz) {
                break;
            }
            last = sz;
        }
        if(sz == packets) {
            break;
        }
        sched_yield();
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    double rate = nbits / (double) elapsed_us / 1000.;
    logger->info("Numbers of bits: {} ", nbits);
    logger->info("Time [us]: {}", elapsed_us);
    logger->info("Throughput [Gbps]: {}", rate);
    rd_cp.finish();

    proc_thread.join();
}
