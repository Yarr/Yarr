// #################################
// # Contacts: Eunchong Kim
// # Email: eunchong.kim at cern.ch
// # Date: April 2019
// # Project: Local Database for Yarr
// # Description: write to DB from directory in a range
// ################################
//
#include <iostream>
#include <cstdlib>
#include <string>
#include "json.hpp"

#include "Database.h"

int main(int argc, char *argv[]){
    //if(argc == 1) {
    //  	std::cout << "No imput json name" << std::endl;
    //  	return -1;
    //}
    //std::cout << "json : " << argv[1] << std::endl;
    //std::string collection = "component";
    //std::string json = argv[1];
    
    Database *database = new Database();
    //database->writeFiles("QU-10_chipID1", 853, 1122);
    database->writeFiles("MA-03", 853, 1122);
    delete database;
    
    return 0;
}

