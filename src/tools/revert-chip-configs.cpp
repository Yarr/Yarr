//////////////////////////////////////////////////
// A utility to revert chip configs to revision
// before YARR scan.
// author: Emily Thompson
// e-mail: emily.anne.thompson@cern.ch
// September 2023
//////////////////////////////////////////////////

// std/stl
#include <iostream>
#include <string>
#include <sstream>
#include <getopt.h>

#include <filesystem>
namespace fs = std::filesystem;
#include <memory> // unique_ptr

// YARR
#include "ScanHelper.h" // openJson
#include "Utils.h"

void print_usage(char* argv[]) {
    std::cerr << " revert-chip-configs" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Usage: " << argv[0] << " [options]" << std::endl;
    std::cerr << " Options:" << std::endl;
    std::cerr << "   -c          Input connectivity JSON file path [required]" << std::endl;
    std::cerr << "   -d          Output directory with YARR scan data [required]" << std::endl;
    std::cerr << "   -i          Position of chip in connectivity file chips list, starting from 0 (default: all chips)" << std::endl;
    std::cerr << "   -n          Chip name (if given will override use of chip index)" << std::endl;
    std::cerr << "   -A          Revert to .after configs instead of .before" << std::endl;
    std::cerr << "   -h|--help   Print this help message and exit" << std::endl;
    std::cerr << std::endl;
}

int main(int argc, char* argv[]) {
    std::string connectivity_filename = "";
    std::string output_yarr_dir = "";
    int chip_idx = -1;
    std::string chip_name = "";
    bool use_chip_name = false;
    std::string suffix = ".before";

    int c = 0;
    while (( c = getopt(argc, argv, "c:d:i:n:Ah")) != -1) {
        switch (c) {
            case 'c' :
                connectivity_filename = optarg;
                break;
            case 'd':
                output_yarr_dir = optarg;
                break; 
            case 'i' :
                try {
                    chip_idx = std::stoi(optarg);
                } catch (std::exception& e) {
                    std::cerr << "ERROR: Chip index must be an integer value (you provided: " << optarg << ")" << std::endl;
                    return 1;
                }
                break;
            case 'n' :
                chip_name = optarg;
                use_chip_name = true;
                break;
            case 'A' :
	        suffix = ".after";
                break;
            case 'h' :
                print_usage(argv);
                return 0;
            default :
                std::cerr << "Invalid option '" << c << "' supplied, aborting" << std::endl;
                return 1;
        } // switch
    } // while

    fs::path connectivity_path{connectivity_filename};
    if(!fs::exists(connectivity_path)) {
        std::cerr << "ERROR: Provided connectivity file (=" << connectivity_filename << ") does not exist" << std::endl;
        return 1;
    }

    fs::path output_yarr_path{output_yarr_dir};
    if(!fs::exists(output_yarr_path)) {
        std::cerr << "ERROR: Provided directory with output yarr scan results (=" << output_yarr_dir << ") does not exist" << std::endl;
        return 1;
    }

    
    // open up the connectivity config to get the list of front-ends
    auto jconn = ScanHelper::openJsonFile(connectivity_filename);
    std::string chip_type = ScanHelper::loadChipConfigs(jconn, false, Utils::dirFromPath(connectivity_filename));

    // Loop through chips in connectivity file
    auto chip_configs = jconn["chips"];
    size_t n_chips = chip_configs.size();
    bool file_replaced = false;
    for (size_t ichip = 0; ichip < n_chips; ichip++) {
        fs::path chip_register_file_path{chip_configs[ichip]["__config_path__"]};
        if(!fs::exists(chip_register_file_path)) {
            std::cerr << "WARNING: Chip config for chip at index " << ichip << " in connectivity file does not exist, skipping (" << chip_register_file_path << ")" << std::endl;
            continue;
        }
 
        // Skip chips if they are not specified
        if (!use_chip_name) {
            if ( (chip_idx >= 0) && (chip_idx != ichip) ) {
               continue;
            }
        }
        if (use_chip_name && (chip_register_file_path.string().find(chip_name)==std::string::npos)){
            continue;
        }

        // Search for config files in YARR scan output directory for each chip
        for (const auto & entry : fs::directory_iterator(output_yarr_dir)){
            std::string output_yarr_file = entry.path().filename().string();
            if (output_yarr_file.find(chip_register_file_path.filename() += suffix) != std::string::npos){

                // Make a copy of current chip config
                size_t found = entry.path().string().find(suffix);
                std::string backup_file = entry.path().string().replace(found, suffix.size(), ".revert");
                std::cout << "Writing backup: " << backup_file << std::endl;
                fs::copy_file(chip_register_file_path, backup_file, fs::copy_options::overwrite_existing);

                // Revert chip config to previous state
                std::cout << "Replacing " << chip_register_file_path.string() << " with " << entry.path().string() << std::endl;
                fs::copy_file(entry, chip_register_file_path, fs::copy_options::overwrite_existing);

                file_replaced = true;
                break;
            }
        } 
    }

    if (!file_replaced){
        std::cout << "Did not find any chip configs with suffix '" << suffix << "' in the provided output YARR scan directory that correspond to the chips listed in the connectivity file. No chip configs were reverted." << std::endl;
    }
    std::cerr << "Done." << std::endl;

    return 0;
}
