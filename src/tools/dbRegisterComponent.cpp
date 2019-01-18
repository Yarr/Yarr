// #################################
// # Author: Arisa Kubota
// # Description: upload to mongoDB
// ################################

#include <iostream>
#include <cstdlib>
#include <string>
#include "json.hpp"

#include "Database.h"

void printHelp();

int main(int argc, char *argv[]){

  std::string dbInfoJsonPath = "";
	std::string dbConnPath = "";
  
  int c;
  while ((c = getopt(argc, argv, "hc:I:")) != -1 ){
    switch (c) {
        case 'h':
            printHelp();
            return 0;
            break;
        case 'c':
            dbConnPath = std::string(optarg);
            break;
        case 'I':
            dbInfoJsonPath = std::string(optarg);
            break;
        case '?':
            if(optopt == 'I'){
                std::cerr << "-> Option " << (char)optopt
                          << " requires a parameter! (Proceeding with default)"
                          << std::endl;
            }else if(optopt == 'c'){
                std::cerr << "-> Option " << (char)optopt
                          << " requires a parameter! Aborting... " << std::endl;
                return -1;
            } else {
                std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
            }
            break;
        default:
            std::cerr << "-> Error while parsing command line parameters!" << std::endl;
            return -1;
    }
  }

  if (dbConnPath.size() == 0) {
      std::cerr << "Error: no connectivity config file given, please specify file name under -c option!" << std::endl;
      std::cerr << "       create config files by running configs/create_fei4b_4chip_config.sh if file does not exist." << std::endl;
      return -1;
  }

	std::cout << "connecitivity config file : " << dbConnPath << std::endl;
	std::cout << "environmental config file : " << dbInfoJsonPath << std::endl;
	std::string collection = "component";

  Database *database = new Database();
  database->setTestRunInfo(dbInfoJsonPath);
	database->registerFromConnectivity(dbConnPath);

  delete database;

	return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -c <conn.json> : Provide connectivity configuration." << std::endl;
    std::cout << " -I <user.json> : Provide environmental configuration, default user = $USER, institution = $HOSTNAME." << std::endl;
}


