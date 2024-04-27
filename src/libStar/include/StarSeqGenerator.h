#ifndef STAR_SEQ_GENERATOR
#define STAR_SEQ_GENERATOR

#include <vector>
#include <string>
#include <ostream>
#include <istream>
#include <cstdint>

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

  /// @brief Check if the internal sequence is empty
  /// @return true if m_sequence has no elements, false otherwise
  bool empty() {return m_sequence.empty();}

  /// @brief Get the LCB command byte sequence
  const std::vector<uint8_t>& getSequence() const {return m_sequence;}

  /// @brief Get the flag if or not the commands are for the FELIX firmware LCB encoder
  bool isFW() const {return m_fw;}

  /// @brief Dump the command sequence to output stream
  /// @param os Output stream. Can be for example std::cout or a std::ofstream object.
  void dump(std::ostream &os);

  /// @brief Dump the command sequence to output file
  /// @param filepath Path of the output file
  void dump(const std::string& filepath);

  /// @brief Load the command sequence bytes from input stream
  /// @param is Input stream. Can be for example a std::ifstream object
  /// @param reset If true, overwrite the existing sequence, otherwise append the new one to the existing one
  void load(std::istream &is, bool reset=true);

  /// @brief Load the command sequence bytes from a input file
  /// @param filepath Input file path.
  /// @param reset If true, overwrite the existing sequence, otherwise append the new one to the existing one.
  void load(const std::string& filepath, bool reset=true);

  /// @brief Count the number of L0A frames in the sequence
  /// @return The number of L0A frames
  unsigned count_l0a();

  /// @brief Count the number of triggers in the sequence
  /// @return The number of triggers
  unsigned count_triggers();

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

  inline static const std::string SWTAG = "# LCB";
  inline static const std::string FWTAG = "# Trickle";

};

#endif