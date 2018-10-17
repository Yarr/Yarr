// #################################
// # Author: Arisa Kubota
// # Description: upload to mongoDB
// ################################

#include <iostream>
#include <cstdlib>
#include <string>
#include "json.hpp"

#include "Database.h"

int main(int argc, char *argv[]){
	if(argc == 1) {
		std::cout << "No imput collection name" << std::endl;
		return -1;
	}
	if(argc == 2) {
		std::cout << "No imput json file name" << std::endl;
		return -1;
	}
	std::cout << "collection : " << argv[1] << " json : " << argv[2] << std::endl;
  Database *database = new Database();
	std::string id = database->uploadFromJson("collection_test","test.json");
	std::cout<<id<<std::endl;

  delete database;
}

