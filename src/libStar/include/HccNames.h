#ifndef YARR_HCC_STAR_NAMES_H
#define YARR_HCC_STAR_NAMES_H

#include "HccCfg.h"

#include <optional>

/**
 * Utility functions for dynamic enum things.
 *
 * Convert to and from strings, list entries.
 */
namespace HccNames {

std::optional<HCCStarRegister> regFromString(const std::string &n);
bool regStringIsValid(const std::string &n);
std::string regToString(HCCStarRegister r);
const std::vector<HCCStarRegister> &listRegs();

std::optional<HCCStarSubRegister> subRegFromString(const std::string &n);
bool subRegStringIsValid(const std::string &n);
std::string subRegToString(HCCStarSubRegister r);
const std::vector<HCCStarSubRegister> &listSubRegs();

}

#endif
