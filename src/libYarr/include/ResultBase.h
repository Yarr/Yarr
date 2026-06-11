#ifndef RESULTBASE_H
#define RESULTBASE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Result Base Container
// # Comment: 
// ################################

#include <string>

#include "LoopStatus.h"

/**
 * Base class for analysis results?
 */
class ResultBase {
    public:
        /** Named result */
        ResultBase(const std::string& arg_name);
        /** Named result associated with point of scan */
        ResultBase(const std::string& arg_name, LoopStatus &stat);
        /** Destructor */
        virtual ~ResultBase();

        /** Return name */
        std::string getName();

        /** Save data in file */
        virtual void toFile(std::string basename, bool header=true) {}
        /** Plot data in file */
        virtual void plot(std::string basename) {}
        
        /** Set axes titles */
        void setAxisTitle(const std::string& x, const std::string& y="y", const std::string& z="z");
        /** Set X-axis title */
        void setXaxisTitle(const std::string&);
        /** Set Y-axis title */
        void setYaxisTitle(const std::string&);
        /** Set Z-axis title */
        void setZaxisTitle(const std::string&);

        /** Retrieve X-axis title */
        std::string getXaxisTitle();
        /** Retrieve Y-axis title */
        std::string getYaxisTitle();
        /** Retrieve Z-axis title */
        std::string getZaxisTitle();

    protected:
        std::string name;
        std::string xAxisTitle;
        std::string yAxisTitle;
        std::string zAxisTitle;
            
        LoopStatus lStat;
    private:
};
#endif
