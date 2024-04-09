#ifndef STAR_SEQ_GENERATOR
#define STAR_SEQ_GENERATOR

#include <vector>
#include <string>

/// @brief LCB command sequence generator
class StarSeqGenerator {

public:

  /// @brief Constructor
  StarSeqGenerator(bool isFW, bool verbose=false) : m_fw(isFW)
  {}

  /// @brief Destructor
  ~StarSeqGenerator() = default;

  /// @brief Print recognized command string formats
  /// @param os Output stream object. Can be std::cout or a std::stringstream object
  /// @param verbose If true, also print examples of various commands
  static void printCommandFormat(std::ostream &os, bool verbose=false);

  /// @brief Parse a command string and add to the byte sequence
  /// @param command A string representation of LCB commands
  /// @param verbose If true, print recognized command formats if parsing fails
  /// @return true if parsing is successful, otherwise false
  bool parseCommandString(const std::string& command, bool verbose=false);

  /// @brief Parse a vector of command strings and add to the byte sequence
  /// @param commands A vector of string representations of LCB commands
  /// @param verbose If true, print recognized command formats if parsing fails
  /// @return true if parsing is successful, otherwise false
  bool parseCommandSequence(const std::vector<std::string>& commands, bool verbose=false);

  /// @brief Remove all command sequence bytes
  void clear() {m_sequence.clear();}

  /// @brief Get the LCB command byte sequence
  const std::vector<uint8_t>& getSequence() const {return m_sequence;}

private:

  void addIdle(unsigned nframes);
  void addIdle(const std::vector<std::string>& cmd_tokens);

  void addL0(uint8_t mask, uint8_t tag, bool bcr=false);
  void addL0(const std::vector<std::string>& cmd_tokens);

  void addFastCmd(uint8_t type, uint8_t delay);
  void addFastCmd(const std::vector<std::string>& cmd_tokens);

  void addRegRdCmd(bool isABC, uint32_t address, uint8_t hccID=0xf, uint8_t abcID=0xf);
  void addRegWrCmd(bool isABC, uint32_t address, uint32_t value, uint8_t hccID=0xf, uint8_t abcID=0xf);
  void addRegCmd(const std::vector<std::string>& cmd_tokens);

  void addCommand(const std::string& command);

  std::vector<uint8_t> m_sequence;

  bool m_fw {false};

};

#endif