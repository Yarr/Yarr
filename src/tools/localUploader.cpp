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
		std::cout << "No imput json name" << std::endl;
		return -1;
	}
	std::cout << "json : " << argv[1] << std::endl;
	std::string collection = "component_test";
	std::string json = argv[1];

  Database *database = new Database();
	std::string id = database->uploadFromJson(collection,json);
	std::cout<<id<<std::endl;

  delete database;

	return 0;
}

