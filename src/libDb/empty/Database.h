#ifndef DATABASE_H
#define DATABASE_H

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Database {
    public:
        Database(std::string i_host_ip = "mongodb://localhost:27017"){};
        ~Database(){};

        void setConnCfg(std::vector<std::string>){};
        void setTestRunInfo(std::string){};
        void write(std::string, std::string, int, std::string){};
        std::string uploadFromJson(std::string, std::string){return "ERROR";};
        void registerFromConnectivity(std::string){};
        void writeFiles(std::string, int, int){};

    protected:
        std::string getValue(std::string, std::string, std::string, std::string, std::string, std::string i_bson_type="string"){return "ERROR";};
        std::string registerComponentTestRun(std::string, std::string, std::string, int){return "ERROR";};
        std::string registerTestRun(std::string, int){return "ERROR";};
        void addComment(std::string, std::string, std::string){};
        void addAttachment(std::string, std::string, std::string, std::string, std::string, std::string, std::string){};
        void addDefect(std::string, std::string, std::string, std::string){};
        std::string writeGridFsFile(std::string, std::string){return "ERROR";};
        std::string writeDatFile(std::string, std::string){return "ERROR";};
        std::string writeJsonCode(json&, std::string){return "ERROR";};
        void writeFromDirectory(std::string, std::string, std::string, std::string i_filter=""){};
        void writeJsonFile(std::string, std::string, std::string, std::string i_filter=""){};
        void addTestRunInfo(std::string){};
        void addUserInstitution(std::string, std::string){};
        void addSys(std::string, std::string){};


    private:
};

#endif
