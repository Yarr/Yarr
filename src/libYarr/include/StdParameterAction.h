#ifndef STD_PARAMETER_ACTION_H
#define STD_PARAMETER_ACTION_H

// #################################
// # Author: Bruce Gallop
// # Project: Yarr
// # Description: Generic named parameter 
// ################################

class StdParameterAction {
    public:
        StdParameterAction() = default;
        virtual ~StdParameterAction() = default;

        const std::string &getParName() const { return parName; }

    protected:
        std::string parName;
};

#endif
