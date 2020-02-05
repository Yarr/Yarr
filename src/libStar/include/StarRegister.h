#ifndef STAR_REGISTER_INCLUDE
#define STAR_REGISTER_INCLUDE

#include <map>
#include <memory>
#include <string>

class SubRegister{
    public:
        SubRegister(){
            m_parentReg = 0;
            m_parentRegAddress = -1;
            m_subRegName = "";
            m_bOffset = 0;
            m_mask = 0;
        }

        SubRegister(uint32_t *reg, int parentRegAddress, std::string subRegName, unsigned bOffset, unsigned mask) {
            m_parentReg = reg;
            m_parentRegAddress = parentRegAddress;
            m_subRegName = subRegName;
            m_bOffset = bOffset;
            m_mask = mask;
        }

        // Get value of field
        const unsigned getValue() {
            unsigned maskBits = (1<<m_mask)-1;
            unsigned tmp = ((*m_parentReg&(maskBits<<m_bOffset))>>m_bOffset);

            return tmp;
        }

        // Write value to field and config
        void updateValue(const uint32_t cfgBits) {
            unsigned maskBits = (1<<m_mask)-1;
            *m_parentReg=(*m_parentReg&(~(maskBits<<m_bOffset))) |
              ((cfgBits&maskBits)<<m_bOffset);
        }

        std::string name() const { return m_subRegName; }

        int getParentRegAddress() const { return m_parentRegAddress; }

        uint32_t getParentRegValue() const { return *m_parentReg; }

    private:
        uint32_t *m_parentReg;
        int m_parentRegAddress;
        std::string m_subRegName;
        unsigned m_bOffset;
        unsigned m_mask;
};

class Register {
    public:
        Register(int addr=-1, uint32_t value=0)
          : m_regAddress(addr),
            m_regValue(value)
        {}

        // need to explicitly default due to unique_ptr
        Register(const Register& other) = default;
        Register(Register && other) = default;
        Register &operator=(const Register& other) = default;
        Register &operator=(Register&& other) = default;

        ~Register() = default;

        int addr() const { return m_regAddress;}
        const uint32_t getValue() const { return m_regValue;}
        void setValue(uint32_t value) {m_regValue = value;}
        void setMySubRegisterValue(std::string subRegName, uint32_t value) {
            subRegisterMap[subRegName]->updateValue(value);
        }

        const unsigned getMySubRegisterValue(std::string subRegName) {
            return subRegisterMap[subRegName]->getValue();
        }

        SubRegister *addSubRegister(std::string subRegName, unsigned bOffset, unsigned mask) {
            subRegisterMap[subRegName].reset(new SubRegister(&m_regValue, m_regAddress, subRegName,bOffset, mask));
            return subRegisterMap[subRegName].get();
        }

    private:
        std::map<std::string, std::unique_ptr<SubRegister>> subRegisterMap;

        int m_regAddress;
        uint32_t m_regValue;
};

#endif
