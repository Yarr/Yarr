#include "HccNames.h"

#include "enum.h"
#include "StarRegDefs.h"

BETTER_ENUM(HCCStarRegisterEnum, int, HCC_STAR_REGS)

BETTER_ENUM(HCCStarSubRegisterEnum, int, HCC_STAR_SUB_REGS)

namespace HccNames {

std::optional<HCCStarRegister> regFromString(const std::string &n)
{
    return (HCCStarRegister)(int)HCCStarRegisterEnum::_from_string(n.c_str());
}

bool regStringIsValid(const std::string &n)
{
    return HCCStarRegisterEnum::_is_valid(n.c_str());
}

std::string regToString(HCCStarRegister r)
{
    return HCCStarRegisterEnum::_from_integral((int)r)._to_string();
}

std::vector<HCCStarRegister> make_reg_list() {
    std::vector<HCCStarRegister> l;
    auto vals = HCCStarRegisterEnum::_values();
    for(auto &v: vals) {
        l.push_back((HCCStarRegister)(int)v);
    }
    return l;
}

const std::vector<HCCStarRegister> &listRegs() {
    static std::vector<HCCStarRegister> regs = make_reg_list();
    return regs;
}

std::optional<HCCStarSubRegister> subRegFromString(const std::string &n)
{
    return (HCCStarSubRegister)(int)HCCStarSubRegisterEnum::_from_string(n.c_str());
}

bool subRegStringIsValid(const std::string &n)
{
    return HCCStarSubRegisterEnum::_is_valid(n.c_str());
}

std::string subRegToString(HCCStarSubRegister r)
{
    return HCCStarSubRegisterEnum::_from_integral((int)r)._to_string();
}

std::vector<HCCStarSubRegister> make_subreg_list() {
    std::vector<HCCStarSubRegister> l;
    auto vals = HCCStarSubRegisterEnum::_values();
    for(auto &v: vals) {
        l.push_back((HCCStarSubRegister)(int)v);
    }
    return l;
}

const std::vector<HCCStarSubRegister> &listSubRegs() {
    static std::vector<HCCStarSubRegister> regs = make_subreg_list();
    return regs;
}

}