// #################################
// # Author: Eunchong Kim
// # Description: Database viewer
// ################################

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
    database->viewer();
    delete database;
    
    return 0;
}

