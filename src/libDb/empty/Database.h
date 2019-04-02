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
        void setUserInstitution(){};
        void setHistoName(std::string){};
        void startTestRun(std::string, std::string, std::string, int, int, int){};
        std::string writeComponentTestRun(std::string, int, int){return "ERROR";};
        void writeConfig(std::string, json&, std::string){}; 
        void writeTestRun(std::string, std::string, std::string, int, std::string, int, int){};
        void writeFiles(std::string, int, int){};
        std::string uploadFromJson(std::string, std::string){return "ERROR";};
        void registerUserInstitution(std::string, std::string, std::string, std::string){};
        void registerFromConnectivity(std::string){};
        void registerEnvironment(std::string, std::string){};
        void writeAttachment(std::string, std::string, std::string){};

    protected:
        std::string getValue(std::string, std::string, std::string, std::string, std::string, std::string i_bson_type="string"){return "ERROR";};
        std::string registerComponentTestRun(std::string, std::string, std::string, int, int, int){return "ERROR";};
        std::string registerTestRun(std::string, int, int, int){};
        void addComment(std::string, std::string, std::string){};
        void addDocument(std::string, std::string, std::string, std::string){};
        void addAttachment(std::string, std::string, std::string, std::string, std::string, std::string, std::string){};
        void addDefect(std::string, std::string, std::string, std::string){};
        void writeFromDirectory(std::string, std::string, std::string, std::string i_filter=""){};
        void addTestRunInfo(std::string){};
        void addUserInstitution(std::string, std::string){};
        void addSys(std::string, std::string){};
        void addVersion(std::string, std::string, std::string, std::string){};
        std::string writeDatFile(std::string, std::string){return "ERROR";};
        std::string writeGridFsFile(std::string, std::string){return "ERROR";};
        std::string writeJsonCode(json&, std::string, std::string){return "ERROR";};

    private:
};

#endif
