#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include "getopt.h"
#include "unistd.h"

#include "AllProcessors.h"
#include "EventData.h"
#include "StarCfg.h"

#include "logging.h"
#include "LoggingConfig.h"

namespace {
auto logger = logging::make_log("benchmark_dataprocessing_star");
}

void run_with_clipboard(StarCfg &cfg, FeDataProcessor &proc, int iterations, std::vector<uint8_t> &buffer);
void run_without_clipboard(StarCfg &cfg, FeDataProcessor &proc, int iterations, std::vector<uint8_t> &buffer);

std::vector<uint8_t> read_file(const std::string &file_name) {
    std::vector<uint8_t> buffer;

    std::error_code ec;
    std::filesystem::path name(file_name);
    auto len = std::filesystem::file_size(name, ec);
    if(ec)  {
        logger->error("Failed to open event data file: {}", file_name);
        return {};
    }
    buffer = std::vector<uint8_t>(len);
    std::ifstream in(name, std::ios::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), len);
    return buffer;
}

void printHelp() {
    std::cout << "Run data process benchmark (star version)\n";
    std::cout << "  benchmark_data_processor [opts] data_file\n";
    std::cout << "   --file-name,-f FILE_NAME\tFile name to load\n";
    std::cout << "   --iterations,-i ITER\tNumber of iterations to run\n";
    std::cout << "   --help,-h Show help\n";
}

int main(int argc, char *argv[]) {
    json loggerConfig;
    loggerConfig["pattern"] = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    loggerConfig["log_config"][0]["name"] = "all";
    loggerConfig["log_config"][0]["level"] = "info";
    loggerConfig["outputDir"] = "";
    logging::setupLoggers(loggerConfig);

    const struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"file-name", required_argument, nullptr, 'f'},
      {"iterations", required_argument, nullptr, 'i'},
      {nullptr, 0, nullptr, 0}
    };

    unsigned iterations = 100;
    std::string file_name;

    int c;
    while((c = getopt_long(argc, argv, "hf:i:", long_options, nullptr)) != -1) {
        switch(c) {
        case 'h':
            printHelp();
            return 0;
        case 'f':
            file_name = optarg;
            break;
        case 'i':
            iterations = atoi(optarg);
            break;
        default:
            std::cerr << "Error while parsing command line arguments!" << std::endl;
            printHelp();
            return 1;
        }
    }

    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("Star");

    if(optind != argc && file_name.empty()) {
        file_name = argv[optind++];
    }

    if(optind != argc) {
        std::cout << "Too many parameters\n";
        printHelp();
        return 0;
    }

    if(file_name.empty()) {
        std::cout << "No file given to load data from\n";
        return 1;
    }

    auto buffer = read_file(file_name);
    if(buffer.empty()) {
      std::cout << "Aborting, buffer empty\n";
      return 1;
    }

    StarCfg cfg(1, 1);
    cfg.setHCCRegister(40, 0x7ff);

    run_with_clipboard(cfg, *proc, iterations, buffer);
    run_without_clipboard(cfg, *proc, iterations, buffer);

    return 0;
}

void run_with_clipboard(StarCfg &cfg, FeDataProcessor &proc, int iterations, std::vector<uint8_t> &buffer) {

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    proc.connect(&cfg, &rd_cp, &em_cp);

    proc.init();

    std::thread proc_thread([&proc]() { proc.process(); });

    uint32_t data_count = *((uint32_t *) &buffer[0]);

    std::size_t nbits{};
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < iterations; i++) {
        uint32_t index = 0;
        // Skip data_count
        index += sizeof(uint32_t);
        // logger->info("Loading {} packets", n);
        std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus({1}, {LOOP_STYLE_MASK})));
        for (unsigned k = 0; k < data_count; k++) {
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
            rdc->add(std::move(rd));
        }

        rd_cp.pushData(std::move(rdc));
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
    logger->info("Throughput with clipboard [Gbps]: {}", rate);
    logger->info("Iterations/s [Hz]: {}", iterations/(elapsed_us*1e-6));
    logger->info("Packets ({})/s [Hz]: {}", data_count, (iterations*data_count)/(elapsed_us*1e-6));
    rd_cp.finish();

    proc_thread.join();
}

void run_without_clipboard(StarCfg &cfg, FeDataProcessor &proc, int iterations, std::vector<uint8_t> &buffer) {

    proc.init();

    std::size_t nbits{};
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    std::size_t done_count = 0;
    auto last_log = std::chrono::steady_clock::now();

    uint32_t n_buffer = *((uint32_t *) &buffer[0]);
    unsigned expected = n_buffer * iterations;

    auto check_log = [&]() {
        auto log_check = std::chrono::steady_clock::now();
        if((log_check - last_log) > std::chrono::milliseconds(100)) {
            logger->info("Progress {}%", done_count*100/expected);
            last_log = log_check;
        }
    };

    for (unsigned i = 0; i < iterations; i++) {
        uint32_t index = 0;

        // Start from scratch on each iteration, but already read the count
        index += sizeof(uint32_t);

        std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus({1}, {LOOP_STYLE_MASK})));
        for (unsigned k = 0; k < n_buffer; k++) {
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

            std::vector<uint32_t> edata((uint32_t *) &buffer[index], ((uint32_t *) &buffer[index]) + (data.nbytes+3)/4);
            nbits += data.nbytes*8;
            index += sizeof(uint8_t) * data.nbytes;
            RawDataPtr rd = std::make_shared<RawData>(0, std::move(edata));

            rdc->add(std::move(rd));
        }

        proc.process_event_core(*rdc, [](auto){});
        done_count ++;

        check_log();
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    double rate = nbits / (double) elapsed_us / 1000.;
    logger->info("Numbers of bits: {} ", nbits);
    logger->info("Time [us]: {}", elapsed_us);
    logger->info("Throughput without clipboard [Gbps]: {}", rate);
    logger->info("Iterations/s [Hz]: {}", iterations/(elapsed_us*1e-6));
    logger->info("Packets ({})/s [Hz]: {}", n_buffer, (iterations*n_buffer)/(elapsed_us*1e-6));
}
