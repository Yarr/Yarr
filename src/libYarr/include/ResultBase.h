#ifndef RESULTBASE_H
#define RESULTBASE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Result Base Container
// # Comment: 
// ################################

#include <iostream>
#include <string>

class ResultBase {
    public:
        ResultBase(std::string arg_name);
        ~ResultBase();

        std::string getName();
        
        void setAxisTitle(std::string x, std::string y="y", std::string z="z");
        void setXaxisTitle(std::string);
        void setYaxisTitle(std::string);
        void setZaxisTitle(std::string);

        std::string getXaxisTitle();
        std::string getYaxisTitle();
        std::string getZaxisTitle();

    protected:
        std::string name;
        std::string xAxisTitle;
        std::string yAxisTitle;
        std::string zAxisTitle;
    private:
};
#endif
