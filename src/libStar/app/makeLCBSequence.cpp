#include <iostream>
#include <string>
#include <getopt.h>
#include <filesystem>
#include <getopt.h>

#include "StarSeqGenerator.h"
#include "ScanHelper.h"

namespace {

  //Generate LCB byte streams from command strings

  void printHelp() {
    std::cout << std::endl;
    std::cout << "Usage: " << std::endl;
    std::cout << " -h: Show this help." << std::endl;
    std::cout << " -j <config.json> : Path to a json config file. The json file is expected to be of the format:" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    \"commands\": [\"CMD1\", \"CMD2\", ...]," << std::endl;
    std::cout << "    \"firmware\": false," << std::endl;
    std::cout << "    \"output\": \"path/to/output/file.txt\"" << std::endl;
    std::cout << "  }" << std::endl;
    std::cout << " -f : Generate the byte stream for the trickle memory of FELIX firmware. Overwrite the field \"firmware\" of the json config file if provided." << std::endl;
    std::cout << " -o : Output file path. Overwrite the field \"output\" of the json config file if provided. If there is no output path specified, the generate byte stream is printed to screen." << std::endl;
    std::cout << " -c 'LCB CMD1' 'LCB CMD2' ... : A list of commands. Overwrite the field \"commands\" of the json config file if provided." << std::endl;
    std::cout << std::endl;

    std::cout << "Recognized commands:" << std::endl;
    StarSeqGenerator::printCommandFormat(std::cout, true);
    std::cout << std::endl << std::endl;
  }
} // end of unnamed namespace

//////////
int main(int argc, char *argv[]) {

  std::string configPath;
  std::vector<std::string> commands;
  bool isFELIXFW {false};
  std::string outputPath;

  const struct option long_options[] = {
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0}
    };

  if (argc == 1) {
    printHelp();
    return 0;
  }

  int c;
  while((c = getopt_long(argc, argv, "hj:c:fo:", long_options, nullptr)) != -1) {
    switch(c) {
    case 'h':
      printHelp();
      return 0;
    case 'j':
      configPath = std::string(optarg);
      break;
    case 'c':
      commands.clear();
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        commands.push_back(std::string(argv[optind]));
      }
      break;
    case 'f':
      isFELIXFW = true;
      break;
    case 'o':
      outputPath = std::string(optarg);
      break;
    default:
      std::cerr << "Error while parsing command line arguments!" << std::endl;
      printHelp();
      return 1;
    }
  }

  json cfg;
  if (not configPath.empty()) {
    try {
      cfg = ScanHelper::openJsonFile(configPath);
    } catch (std::runtime_error &e) {
      std::cerr << "Failed to open config file: " << configPath << std::endl;
      std::cerr << e.what() << std::endl;
      return 1;
    }
  }

  if (commands.empty() and cfg.contains("commands")) {
    for (size_t i = 0; i < cfg["commands"].size(); i++) {
      commands.push_back(cfg["commands"][i]);
    }
  }

  if (not isFELIXFW and cfg.contains("firmware")) {
    isFELIXFW = cfg["firmware"];
  }

  if (outputPath.empty() and cfg.contains("output")) {
    outputPath = cfg["output"];
  }

  StarSeqGenerator seqGen(isFELIXFW);
  bool success = seqGen.parseCommandSequence(commands);
  if (success) {

    if (outputPath.empty()) {
      // Print to cout
      seqGen.dump(std::cout);
    } else {
      // Write to file
      seqGen.dump(outputPath);
    }

    return 0;

  } else {

    std::cerr << "Failed to parse commands" << std::endl;
    std::cout << "Recognized commands:" << std::endl;
    StarSeqGenerator::printCommandFormat(std::cout, true);
    std::cout << std::endl;

    return 1;
  }
}