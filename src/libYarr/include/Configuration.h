#ifndef YARR_CONFIGURATION_BACKEND_H
#define YARR_CONFIGURATION_BACKEND_H

#include "storage.hpp"

/**
 * Provide access to configuration information.
 *
 * This proxies requests for information about the following:
 * * Connectivity
 * * FrontEnd configuration
 * * Hardware controller information
 *
 * For now it covers only reading data in json form.
 */
class Configuration {
 public:
    virtual ~Configuration() = default;

    /// Report status via logger
    virtual void reportStatus() = 0;

    /**
     * Return connectivity
     *
     * This is a vector of information about connectivity.
     * Each entry corresponds to information about one front end.
     * The response should be the same length as the input.
     */
    virtual json getConnectivity(const std::vector<std::string> &names) = 0;

    /**
     * Return configuration for one FrontEnd.
     *
     * This returns the configuration for the named FrontEnd.
     *
     * If the response is a string == "default", then the default
     * configuration for the required front end should be used instead.
     */
    virtual json getFrontEndConfig(const std::string &name) = 0;
};

#endif
