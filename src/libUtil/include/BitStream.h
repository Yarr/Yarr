#ifndef BITSTREAM_H
#define BITSTREAM_H

/********************************
 * Bitstream
 * Author: Carlos.Solans@cern.ch
 * Description: Manipulate bits
 * Date: October 2017
 *********************************/

#include <cstdint>
#include <vector>
#include <string>

/**
 * @brief BitStream is a helper class to handle bit manipulations.
 *
 * BitStream::FromBinString loads a binary string stream.
 * BitStream::FromHexString loads a hexadecimal string stream.
 * Bits are added to the end of the BitStream with BitStream::Add
 * or accessed directly with BitStream::Set.
 * Single bits or a range of bits can be accessed with BitStream::Get.
 * In case a range of bits is accessed, a new BitStream will be retuned.
 * Total number of bits returned by BitStream::GetNbits.
 * To return the bits as a number of integers, use BitStream::GetWord.
 * The number of words is returned with BitStream::GetSize.
 * Requires calling BitStream::Pack before that.
 * Compare with another BitStream with BitStream::Equals.
 * Contents can be cleared with BitStream::Clear.
 *
 * @code
 * BitStream cmd;
 *
 * cmd.Add(5,0x16);
 * cmd.Add(4,0x8).Add(4,0x1);
 * cmd.Equals(BitStream("1101110000001"));
 * cmd.ToHexString();
 * cmd.Clear();
 * @endcode
 *
 **/
class BitStream {
 public:
 
  /**
   * Build an emtpy BitStream
   **/
  BitStream();

  /**
   * Build a BitStream and fill it with the contents of the bit stream.
   * @param bitstring string representation of a bit stream
   **/
  BitStream(std::string bitstring);

  /**
   * Clear the contents and destroy the object
   **/
  ~BitStream();
  
  /**
   * @brief clears the bitstream contents
   **/
  void Clear();
  
  /**
   * @brief compare this BitStream with another one
   * @param b BitStream to compare with
   **/
  bool Equals(const BitStream& b);

  /**
   * @brief adds new bits to the end of the stream
   * @param size uint32_t size of the bits to add
   * @param value uint32_t value of the bits to add starting from LSB
   * @return reference to the same object
   **/
  BitStream & Add(uint32_t size, uint32_t value);
  
  /**
   * @brief sets a bit in a given position, resizes the stream if needed
   * @param pos uint32_t pos of the bits to set
   * @param value bool value of the bits to add starting from LSB
   * @return reference to the same object
   **/
  BitStream & Set(uint32_t pos, bool value);
  
  /**
   * @brief sets a size of bits in a given position, resizes the stream if needed
   * @param pos uint32_t pos of the bits to set
   * @param size uint32_t size of the bits to set
   * @param value uint32_t value of the bits to add starting from LSB
   * @return reference to the object
   **/
  BitStream & Set(uint32_t pos, uint32_t size, uint32_t value);

  /**
   * @brief get a part of the stream as a new BitStream
   * @param pos uint32_t pos of the bits to get
   * @param size uint32_t size of the bits to get
   * @return a new BitStream containing the requested contents.
   **/
  BitStream Get(uint32_t pos, uint32_t size);
  
  /**
   * @brief get a bit of the stream as a boolean
   * @param pos uint32_t pos of the word
   * @return the boolean value of the bit
   **/
  bool Get(uint32_t pos) const;
  
  /**
   * @brief get a part of the stream as fractions of 32-bit words starting from MSB
   * @param pos uint32_t pos of the word
   * @return uint32_t word if pos is within GetSize(), else 0
   **/
  uint32_t GetWord(uint32_t pos);
  
  /**
   * @brief get the number of 32-bit words in the stream
   * @return uint32_t size of stream in units of 32-bit words
   **/
  uint32_t GetSize();  
  
  /**
   * @brief get the number of bits in the stream
   * @return uint32_t the number of bits
   **/
  uint32_t GetNbits() const;

  /**
   * @brief pad to the left
   * Remove leading zeroes
   **/
  BitStream & Pad();

  /**
   * @brief converts the stream into 32-bit words
   * This command is necessary before BitStream::GetWord
   **/
  void Pack();

  /**
   * @brief converts a binary string into a stream
   * @param binStr the string to parse
   **/
  void FromString(std::string binStr);

  /**
   * @brief converts a binary string into a stream
   * @param binStr the string to parse
   **/
  void FromBinString(std::string binStr);

  /**
   * @brief converts a string of hex into a stream
   * @param hexStr the string to parse
   **/
  void FromHex(std::string hexStr);

  /**
   * @brief converts a string of hex into a stream
   * @param hexStr the string to parse
   **/
  void FromHexString(std::string hexStr);

  /**
   * @brief converts a string of hex into a stream
   * @param hexStr the string to parse
   **/
  void ParseHex(std::string hexStr);

  /**
   * @brief get the binary string representation of the stream
   * @return string representation of the stream
   **/
  std::string ToString();

  /**
   * @brief get the binary string representation of the stream
   * @return string representation of the stream
   **/
  std::string ToBinString();

  /**
   * @brief get the hexadecimal string representation of the stream
   * @return string representation of the stream
   **/
  std::string ToHexString();
  
  
 private:
   
  //! Internal vector of bits
  std::vector<bool> m_bits;
  
  //! Internal vector of integers allocated after pack
  std::vector<uint32_t> m_words;

};

#endif
