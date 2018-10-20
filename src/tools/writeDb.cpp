// #################################
// # Author: Arisa Kubota
// # Description: upload to mongoDB
// ################################
// ./bin/writeDb -j <json_file> -c <collection>

#include <iostream>
#include <cstdlib>
#include <string>
#include "json.hpp"

#include "Database.h"

void printHelp();

int main(int argc, char *argv[]){
	// default collection name
	std::string collectionName = "component_test";
	// default json name
	std::string jsonName = "default.json";

  int c;
	while ((c = getopt(argc, argv, "hj:c:")) != -1){
    switch (c) {
  		case 'h':
  		    printHelp();
  				return 0;
  				break;
  	  case 'j':
			    jsonName = std::string(optarg);
					break;
			case 'c':
			    collectionName = std::string(optarg);
					break;
			case '?':
					if(optopt == 'j' || optopt == 'c'){
				    std::cerr << "-> Option " << (char)optopt
											<< " requires a parameter! (Proceeding with default)"
											<< std::endl;
					}else{
						std::cerr << "-> Unknown parameter: " << (char)optopt 
											<< ". Aborting... " 
											<< std::endl;
						return -1;
					}
					break;
			default:
			    std::cerr << "-> Error while parsing command line parameters!" << std::endl;
					return -1;
		}
	}

	if(jsonName.find(".json")!=std::string::npos){
		std::ifstream jsonFile(jsonName);
		if(!jsonFile){
			std::cout << "#ERROR# Could not find json file: \"" << jsonName << "\". Aborting ... " << std::endl;
			return -1;
		}
	}else{
			std::cout << "#ERROR# No matchng to json file: \"" << jsonName << "\". Aborting ... " << std::endl;
			return -1;
	}

  std::cout << "Write to Yarr Database ... " << std::endl;
  std::cout << "  - json file  : " << jsonName << std::endl;
	std::cout << "  - collection : " << collectionName << std::endl;

  Database *database = new Database();
	std::string id = database->uploadFromJson(collectionName,jsonName);
	std::cout<<id<<std::endl;

  delete database;

	return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -j <upload.json> : set json file to upload." << std::endl;
    std::cout << " -c <collection name> : set collection type" << std::endl;
		std::cout << "    defalt : component_test" << std::endl;
}
