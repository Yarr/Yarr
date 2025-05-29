#include "AbcNames.h"

#include "enum.h"
#include "StarRegDefs.h"

BETTER_ENUM(ABCStarRegisterEnum, int, ABC_STAR_REGS)

BETTER_ENUM(ABCStarSubRegisterEnum, int, ABC_STAR_SUB_REGS)

namespace AbcNames {

std::optional<ABCStarRegister> regFromString(const std::string &n)
{
    return (ABCStarRegister)(int)ABCStarRegisterEnum::_from_string(n.c_str());
}

bool regStringIsValid(const std::string &n)
{
    return ABCStarRegisterEnum::_is_valid(n.c_str());
}

std::string regToString(ABCStarRegister r)
{
    return ABCStarRegisterEnum::_from_integral((int)r)._to_string();
}

std::vector<ABCStarRegister> make_reg_list() {
    std::vector<ABCStarRegister> l;
    auto vals = ABCStarRegisterEnum::_values();
    for(auto &v: vals) {
        l.push_back((ABCStarRegister)(int)v);
    }
    return l;
}

const std::vector<ABCStarRegister> &listRegs() {
    static std::vector<ABCStarRegister> regs = make_reg_list();
    return regs;
}

std::optional<ABCStarSubRegister> subRegFromString(const std::string &n)
{
    return (ABCStarSubRegister)(int)ABCStarSubRegisterEnum::_from_string(n.c_str());
}

bool subRegStringIsValid(const std::string &n)
{
    return ABCStarSubRegisterEnum::_is_valid(n.c_str());
}

std::string subRegToString(ABCStarSubRegister r)
{
    return ABCStarSubRegisterEnum::_from_integral((int)r)._to_string();
}

std::vector<ABCStarSubRegister> make_subreg_list() {
    std::vector<ABCStarSubRegister> l;
    auto vals = ABCStarSubRegisterEnum::_values();
    for(auto &v: vals) {
        l.push_back((ABCStarSubRegister)(int)v);
    }
    return l;
}

const std::vector<ABCStarSubRegister> &listSubRegs() {
    static std::vector<ABCStarSubRegister> regs = make_subreg_list();
    return regs;
}

}
