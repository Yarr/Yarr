#ifndef YARR_ABC_STAR_NAMES_H
#define YARR_ABC_STAR_NAMES_H

#include "AbcCfg.h"

#include <optional>

/**
 * Utility functions for dynamic enum things.
 *
 * Convert to and from strings, list entries.
 */
namespace AbcNames {

std::optional<ABCStarRegister> regFromString(const std::string &n);
bool regStringIsValid(const std::string &n);
std::string regToString(ABCStarRegister r);
const std::vector<ABCStarRegister> &listRegs();

std::optional<ABCStarSubRegister> subRegFromString(const std::string &n);
bool subRegStringIsValid(const std::string &n);
std::string subRegToString(ABCStarSubRegister r);
const std::vector<ABCStarSubRegister> &listSubRegs();

}

#endif
