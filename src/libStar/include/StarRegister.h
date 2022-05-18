#ifndef STAR_REGISTER_INCLUDE
#define STAR_REGISTER_INCLUDE

#include <map>
#include <memory>
#include <string>

/// Description of a sub register
struct SubRegisterInfo {
        SubRegisterInfo(int regAddress, std::string subRegName, unsigned bOffset, unsigned width)
          : m_regAddress(regAddress),
            m_subRegName(subRegName),
            m_bOffset(bOffset),
            m_width(width)
        {}

        int m_regAddress;
        unsigned m_bOffset;
        unsigned m_width;

        std::string name() const { return m_subRegName; }

        int getRegAddress() const { return m_regAddress; }

    private:
        std::string m_subRegName;
};

/// A sub register of register, const implies the value can't be changed
class ConstSubRegister {
    public:
        ConstSubRegister(const uint32_t *reg, std::shared_ptr<const SubRegisterInfo> info)
          : m_info(info), m_parentReg(reg) {}

        // Get value of field
        unsigned getValue() const {
            unsigned maskBits = (1<<m_info->m_width)-1;
            unsigned tmp = ((*m_parentReg&(maskBits<<m_info->m_bOffset))>>m_info->m_bOffset);

            return tmp;
        }

        uint32_t getParentRegValue() const{ return *m_parentReg; }

   private:
        std::shared_ptr<const SubRegisterInfo> m_info;
        const uint32_t *m_parentReg;
};

/// A sub register of register, this one can update the value
class SubRegister {
    public:
        SubRegister(uint32_t *reg, std::shared_ptr<const SubRegisterInfo> info)
          : m_info(info),
            m_parentReg(reg)
        {
        }

        // Get value of field
        unsigned getValue() const {
            unsigned maskBits = (1<<m_info->m_width)-1;
            unsigned tmp = ((*m_parentReg&(maskBits<<m_info->m_bOffset))>>m_info->m_bOffset);

            return tmp;
        }

        // Write value to field and config
        void updateValue(const uint32_t cfgBits) {
            unsigned maskBits = (1<<m_info->m_width)-1;
            if(cfgBits & ~maskBits) {
                throw std::runtime_error("Attempt to write invalid bits in sub register");
            }
            *m_parentReg=(*m_parentReg&(~(maskBits<<m_info->m_bOffset))) |
              ((cfgBits&maskBits)<<m_info->m_bOffset);
        }

        uint32_t getParentRegValue() const{ return *m_parentReg; }

   private:
        std::shared_ptr<const SubRegisterInfo> m_info;
        uint32_t *m_parentReg;
};

struct RegisterInfo {
        RegisterInfo(int address)
            : m_regAddress(address)
        {}

        int m_regAddress;

        int addr() const{ return m_regAddress;}

        std::shared_ptr<const SubRegisterInfo> addSubRegister(std::string subRegName, unsigned bOffset, unsigned mask) {
            subRegisterMap[subRegName].reset(new SubRegisterInfo(m_regAddress, subRegName, bOffset, mask));
            return subRegisterMap[subRegName];
        }

        std::shared_ptr<const SubRegisterInfo> getSubRegister(std::string subRegName) const {
            try {
                return subRegisterMap.at(subRegName);
            } catch(std::out_of_range &e) {
                throw std::runtime_error("RegisterInfo: Bad sub-register lookup"); 
            }
        }
private:
        std::map<std::string, std::shared_ptr<const SubRegisterInfo>> subRegisterMap;
};

class Register {
    public:
        Register(std::shared_ptr<const RegisterInfo> info, uint32_t value)
          : m_info(info),
            m_regValue(value)
        {}

        // need to explicitly default due to unique_ptr
        Register(const Register& other) = default;
        Register(Register && other) = default;
        Register &operator=(const Register& other) = default;
        Register &operator=(Register&& other) = default;

        ~Register() = default;

        int addr() const { return m_info->m_regAddress;}
        const uint32_t getValue() const { return m_regValue;}
        void setValue(uint32_t value) {m_regValue = value;}
        void setMySubRegisterValue(std::string subRegName, uint32_t value){
            auto info = m_info->getSubRegister(subRegName);
            SubRegister(&m_regValue, info).updateValue(value);
        }

        const unsigned getMySubRegisterValue(std::string subRegName){
            auto info = m_info->getSubRegister(subRegName);
            return SubRegister(&m_regValue, info).getValue();
        }

        ConstSubRegister getSubRegister(std::shared_ptr<const SubRegisterInfo> info) const {
            return ConstSubRegister(&m_regValue, info);
        }

        SubRegister getSubRegister(std::shared_ptr<const SubRegisterInfo> info) {
          return SubRegister(&m_regValue, info);
        }

   private:
        std::shared_ptr<const RegisterInfo> m_info;
        uint32_t m_regValue;
};

#endif
