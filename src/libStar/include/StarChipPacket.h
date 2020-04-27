#ifndef STARCHIPPACKET_H
#define STARCHIPPACKET_H

//Class to define an HCC output packet
//StarChipPacket holds a list of 8-bit words, and can parse them to according to the packet type
//StarChipPacketContainer can hold a vector of StarChipPackets
//Cluster structure holds address, next 3-bits, and last cluster bit for ABC clusters
//ErrorBlock holds HCCStar error information and can parse it

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdlib.h>
#include <bitset>
#include <cstring>
#include <sstream>

#include "logging.h"

//Allowed response packet types
enum PacketType {
  TYP_NONE,
  TYP_PR,
  TYP_LP,
  TYP_ABC_RR,
  TYP_ABC_TRANSP,
  TYP_HCC_RR,
  TYP_ABC_FULL,
  TYP_ABC_HPR,
  TYP_HCC_HPR,
  TYP_UNKNOWN
};

//Map of packet type to 4-bit header
static std::map<int, PacketType> packet_type_headers = {
 {1, TYP_PR}, {2, TYP_LP}, {4, TYP_ABC_RR}, {7, TYP_ABC_TRANSP},
 {8, TYP_HCC_RR}, {11, TYP_ABC_FULL}, {13, TYP_ABC_HPR}, {14, TYP_HCC_HPR}, {0, TYP_NONE}
};

//Reverse map of packet type to 4-bit header
static std::map<PacketType, int> packet_type_headers_reversed = {
 {TYP_PR, 1}, {TYP_LP, 2}, {TYP_ABC_RR, 4}, {TYP_ABC_TRANSP, 7},
 {TYP_HCC_RR, 8}, {TYP_ABC_FULL, 11}, {TYP_ABC_HPR, 13}, {TYP_HCC_HPR, 14}, {TYP_NONE, 0}
};

//Map of packet type to packet name
static std::map<PacketType, std::string> packet_type_names = {
 {TYP_NONE, "TYP_NONE"},
 {TYP_PR, "TYP_PR"},
 {TYP_LP, "TYP_LP"},
 {TYP_ABC_RR, "TYP_ABC_RR"},
 {TYP_ABC_TRANSP, "TYP_ABC_TRANSP"},
 {TYP_HCC_RR, "TYP_HCC_RR"},
 {TYP_ABC_FULL, "TYP_ABC_FULL"},
 {TYP_ABC_HPR, "TYP_ABC_HPR"},
 {TYP_HCC_HPR, "TYP_HCC_HPR"},
 {TYP_UNKNOWN, "TYP_UNKNOWN"}

};

//ABC cluster object holding relevant information
struct Cluster{
  int input_channel = 0;
  int raw_cluster = 0;
  int address = 0;
  int next = 0;
};

//Definition of equality for clusters
inline bool operator==(const Cluster& lhs, const Cluster& rhs)
{
    return ( (lhs.input_channel == rhs.input_channel) &&
             (lhs.raw_cluster == rhs.raw_cluster) );
}

//A class defining the error block for recording which channels have packet error
class ErrorBlock{
  public:

  std::vector<int> error_abc;
  std::vector<int> error_bcid;
  std::vector<int> error_l0tag;
  std::vector<int> error_timeout;

  ErrorBlock(uint64_t error_word){
    this->parse(error_word);
  }

  ~ErrorBlock(){
    this->clear();
  }

  //Clear list of input channels for each error
  void clear(){
    error_abc.clear();
    error_bcid.clear();
    error_l0tag.clear();
    error_timeout.clear();
  }

  //Print the list of input channels for which each error was received
  void print(std::ostream &os){
    os << "Received packet errors for the following channels:\n";
    os << "  ABC Error: ";
    for(unsigned int iE=0; iE < error_abc.size(); ++iE){
      os << error_abc.at(iE) << " ";
    }
    os << "\n  BCID Error: ";
    for(unsigned int iE=0; iE < error_bcid.size(); ++iE){
      os << error_bcid.at(iE) << " ";
    }
    os << "\n  L0tag Error: ";
    for(unsigned int iE=0; iE < error_l0tag.size(); ++iE){
      os << error_l0tag.at(iE) << " ";
    }
    os << "\n  Timeout Error: ";
    for(unsigned int iE=0; iE < error_timeout.size(); ++iE){
      os << error_timeout.at(iE) << " ";
    }
    os << "\n";
  }

  void parse(uint64_t error_word){

    this->clear();
    uint16_t word;

    //ABCStar error indicated
    word = (error_word >> 36) & 0x7FF;
    for(unsigned int i_chan=0; i_chan < 11; ++i_chan){
      if( (word >> i_chan) & 1 )
        this->error_abc.push_back(i_chan);
    }
    //BCID Mismatch
    word = (error_word >> 24) & 0x7FF;
    for(unsigned int i_chan=0; i_chan < 11; ++i_chan){
      if( (word >> i_chan) & 1 )
        this->error_bcid.push_back(i_chan);
    }
    //L0tag Mismatch
    word = (error_word >> 12) & 0x7FF;
    for(unsigned int i_chan=0; i_chan < 11; ++i_chan){
      if( (word >> i_chan) & 1 )
        this->error_l0tag.push_back(i_chan);
    }
    //Input Channel timeout
    word = (error_word >> 0) & 0x7FF;
    for(unsigned int i_chan=0; i_chan < 11; ++i_chan){
      if( (word >> i_chan) & 1 )
        this->error_timeout.push_back(i_chan);
    }
  }

};


//HCCStar Packet class, holding information for and parsing various packet types
//Corresponds to a single HCCStar Packet
class StarChipPacket{
  static logging::Logger &logger() {
    static logging::LoggerStore instance = logging::make_log("Star::StarChipPacket");
    return *instance;
  }

  public:

  //Raw 8-bit words added to the packet
  std::vector<uint16_t> raw_words;
  //Packet type, parsed from raw words
  PacketType type = TYP_NONE;
  //Error block (empty if no errors)
  std::unique_ptr<ErrorBlock> error_block;

  //HCC or ABC read information
  unsigned char address = 0;
  unsigned int value = 0;
  uint16_t channel_abc = 0; //IC number, for ABC responses only
  uint16_t abc_status = 0;  // Status word from ABC

  //LP / PR information
  int flag = 0;
  int bcid = 0;
  int bcid_parity = 0;
  int l0id = 0;
  std::vector<Cluster> clusters;
  int num_idles = 0;

  //Empty constructor
  StarChipPacket() {
    raw_words.clear();
    clusters.clear();
  }

  ~StarChipPacket(){
    this->clear();
  }

  bool is_parsed(){
    if( this->type == TYP_NONE )
      return false;
    else
      return true;
  }

  //Add a new 8-bit words
  void add_word(uint16_t word){
    raw_words.push_back(word);
  }

  //Return number of 8-bit words in the packet
  unsigned int n_words(){
    return raw_words.size();
  }

  //Return number of clusters in the packet
  unsigned int n_clusters(){
    return clusters.size();
  }

  //Return type of packet
  PacketType getType(){
   return type;
  }

  //If packet words are empty
  bool is_empty(){
    if( raw_words.size() == 0 )
      return true;
    else
      return false;
  }

  //Print basic info
  void print(std::ostream &os) {
    if(this->type == TYP_LP || this->type == TYP_PR){
      os << "Packet info: BCID " << bcid << " (" << bcid_parity << "), "
         << "L0ID " << l0id << ", nClusters " << this->clusters.size() << "\n";
      if(this->error_block)
        this->error_block->print(os);
    }else if(this->type == TYP_ABC_RR || this->type == TYP_HCC_RR || this->type == TYP_ABC_HPR || this->type == TYP_HCC_HPR ){
      os << std::hex << std::setfill('0');
      os << "Packet info: Address " << std::setw(2) << (unsigned)address
         << ", Value " << std::setw(8) << value << "\n";
      os << std::dec << std::setfill(' ');
    }
  }

  void print_more(std::ostream &os) {
    std::string type_name = packet_type_names[this->type];
    os << "Packet type " << type_name << ", ";
    if(this->type == TYP_LP || this->type == TYP_PR){
      os << "BCID " << bcid << " (" << bcid_parity << "), L0ID " << l0id
         << ", nClusters " << this->clusters.size();
    }else if(this->type == TYP_ABC_RR || this->type == TYP_HCC_RR || this->type == TYP_ABC_HPR || this->type == TYP_HCC_HPR ){
      os << "ABC " << channel_abc << ", Address ";
      os << std::hex << std::setfill('0');
      os << std::setw(2) << (unsigned)address << ", Value " << std::setw(8) << value;
      os << std::dec << std::setfill(' ');
    }
    os << "\n";
    if(this->error_block)
      this->error_block->print(os);
  }

  //Print clusters (for LP / PR packets)
  void print_clusters(std::ostream &os) {
    this->print(os);
    os << "Packet's abc clusters are:\n";
    for(unsigned int iW=0; iW < clusters.size(); ++iW){
      Cluster cluster = clusters.at(iW);
      std::string next_binary = std::bitset<3>(cluster.next).to_string();
      os << "  " << iW << ") InputChannel: " << cluster.input_channel
         << ", Address: 0x";
      os << std::hex << std::setfill('0');
      os << std::setw(2) << cluster.address;
      os << std::dec << std::setfill(' ');
      os << ", Next Strip Pattern: " << next_binary << ".\n";
    }
    os << "\n";
  }

  void print_raw_clusters(std::ostream &os){
    os << "Packet's abc clusters are:\n";
    for(unsigned int iW=0; iW < clusters.size(); ++iW){
      Cluster cluster = clusters.at(iW);
      os << "  " << iW << ") InputChannel: " << cluster.input_channel
         << ", Raw Cluster 0x";
      os << std::hex << std::setfill('0');
      os << std::setw(3) << cluster.raw_cluster << ".\n";
    }
    os << "\n";
  }

  //Print all raw words in packet
  void print_words(std::ostream &os) {
    os << "Packet's " << raw_words.size() << " raw 10b words are: ";
    os << std::hex << std::setfill('0');
    for(unsigned int iW=0; iW < raw_words.size(); ++iW){
      os << std::setw(3) << raw_words.at(iW) << " ";
    }
    os << std::dec << std::setfill(' ');
    os << "\n";
  }

  //Return a string of the raw words in a single line
  std::string raw_word_string(){
    std::string raw_words_str = "";
    char text_buffer[10];
    for(unsigned int iW=0; iW < raw_words.size(); ++iW){
      sprintf(text_buffer, "%03x ", raw_words.at(iW));
      raw_words_str += std::string(text_buffer);  memset(text_buffer, 0, sizeof text_buffer);
    }
    return raw_words_str;
  }

  //Clear only parsed information
  void clear_parsed_info(){
    this->address = 0;
    this->value = 0;
    this->channel_abc = 0;
    this->type = TYP_NONE;

    this->error_block.reset();

    this->flag = 0;
    this->bcid = 0;
    this->bcid_parity = 0;
    this->l0id = 0;
    this->clusters.clear();
  }

  //Clear raw words and parsed information
  void clear(){
    this->raw_words.clear();
    this->clear_parsed_info();
    this->num_idles = 0;
  }

  //Parse a HCC read packet
  int parse_data_HCC_read(){
    if( raw_words.size() != 8 ){
      if((raw_words.size()-2)/4 == 2) {
        // Transfer of data is 32bit, not including SOP/EOP, so don't have precise length
        logger().debug("HCC readout packet should be 8 ten-bit words, but this is {}. Allowing small mismatch.", raw_words.size());
      } else {
        logger().error("Error, HCC readout packet should be 8 ten-bit words, but this is {}. Not parsing packet.", raw_words.size());
        return 1;
      }
    }

    this->address = ((raw_words[1] & 0xF) << 4) | ((raw_words[2] >> 4) & 0xF);
    this->value = ((raw_words[2] & 0xF) << 28) | ((raw_words[3] & 0xFF) << 20) |
      ((raw_words[4] & 0xFF) << 12) | ((raw_words[5] & 0xFF) << 4) | ((raw_words[6] >> 4) & 0xF) ;

  return 0;
  }

  //Parse an ABC read packet
  int parse_data_ABC_read(){
    if( raw_words.size() != 11 ){
      if((raw_words.size()-2)/4 == 3) {
        // Transfer of data is 32bit, not including SOP/EOP, so don't have precise length
        logger().debug("ABC readout packet should be 11 ten-bit words, but this is {}. Allowing small mismatch.", raw_words.size());
      } else {
        logger().error("ABC readout packet should be 11 ten-bit words, but this is {}. Not parsing packet.", raw_words.size());
        return 1;
      }
    }

    this->channel_abc = raw_words[1] & 0xF;
    this->address = raw_words[2] & 0xFF;
    // Top nibble of raw_words[3] should be all 0
    this->value = ((raw_words[3] & 0xF) << 28)
                | ((raw_words[4] & 0xFF) << 20)
                | ((raw_words[5] & 0xFF) << 12)
                | ((raw_words[6] & 0xFF) << 4)
                | ((raw_words[7] & 0xF0) >> 4);

    this->abc_status = ((raw_words[7] & 0xF) << 12)
                     | ((raw_words[8] & 0xFF) << 4)
                     | ((raw_words[9] & 0xF0) >> 4);

    return 0;
  }

  //Parse a PR or LP packet
  int parse_data_PRLP(){
    this->flag = (raw_words[1] >> 3) & 1;
    this->l0id = ((raw_words[1] & 0b111) << 4) | ((raw_words[2] >> 4) & 0xF);
    this->bcid = (raw_words[2] >> 1 & 0x7);
    this->bcid_parity = (raw_words[2] & 0x1);

    uint16_t word = 0;
    bool continue_parsing = true;
    unsigned int iW=3;
    while( continue_parsing ){

      if( iW+1 >= raw_words.size() ){
        logger().error("Error, we somehow reached end of words without the end pattern!");
        return 1;
      }

      //Ignore HCC idles
      while( raw_words[iW] == 0x3FF ){
        if( iW+2 >= raw_words.size() ){
          logger().error("Error, data ends in HCC idles (0x3FF) without giving an end of physics packet 0x6FED!");
          return 1;
        }else{ //skip the idle
          iW++;
        }
      }

      word = (raw_words[iW] << 8) | raw_words[iW+1];
      //printf("Looking at word %x\n", word);

      if(word == 0x77F4){
        logger().info("We received an error block 0x77F4!");

        if( raw_words.size() < iW+8 ){
          logger().error("Error, we expect eight 8-bit error block words, but only found {} words!", raw_words.size()-iW);
          return 1;
        }

        //printf("Looking at %x vs %lx, then shifted to %lx\n", raw_words[iW+2], uint64_t(raw_words[iW+2]), (uint64_t(raw_words[iW+2]) << 40) );
        uint64_t error_word = 0;
        for(unsigned int i=0; i < 6; ++i){
          //word index increments (2->7), amount of shift decrements (40->0)
          error_word |= uint64_t(raw_words[iW+2+i]) << (8*(5-i));
        }

        //Creates error block and parses from word
        this->error_block.reset(new ErrorBlock(error_word));
        iW += 8;

      }else if(word == 0x6FED){ //End of physics packet
        continue_parsing = false;
      }else if((word&0x7FF) == 0x3FE){ //No cluster (from ABC)
        // NB Is useful to put this into the output
        // That would mainly be for testing purposes
        iW += 2; //Ignore this cluster
      }else{
        Cluster cluster = Cluster();
        cluster.input_channel = (word >> 11) & 0xF;
        cluster.raw_cluster = word & 0x7FF;
        cluster.address = (word >> 3) & 0xFF;
        cluster.next = word & 0x7;
        this->clusters.push_back( cluster );
        iW += 2; //increment current word
      }

    }//while continue_parsing

    // iW is first byte of trailer word, and we know about SOP and EOP
    unsigned int used_count = iW + 3;
    unsigned int unused_count = raw_words.size() - used_count;
    if(unused_count > 0) {
      if(unused_count > 2) {
        logger().error("Cluster packet has too many words at the end, trailer at {} of {}", (iW+1), raw_words.size());
        return 1;
      } else {
        logger().debug("Cluster packet has some words at the end, trailer at {} of {}, but allowing small mismatch.", (iW+1), raw_words.size());
      }
    }

    return 0;
  }

  int parse_data_ABC_transparent(){
    // This should be a single wrapped data packet from ABC
    this->channel_abc = raw_words[1] & 0xf;

    std::bitset<64> bits(raw_words[2]);
    for(int i=0; i<7; i++) {
      bits <<= 8;
      bits |= raw_words[3+i];
    }

    logger().debug("ABC transparent {}: {}",
                   this->channel_abc, bits.to_string());

  return 0;
  }

  //------ Function to parse raw data ------//
  //Ensure SOP and EOP exists, parses packet type, then calls appropriate parsing function
  int parse(){
    if(this->is_parsed()){
      logger().warn("Packet was previously parsed, clearing and re-parsing!");
      this->clear_parsed_info();
    }

    std::stringstream oss;
    this->print_words(oss);
    logger().debug("Parsing HCC packet, {}", oss.str());

    if( raw_words.size() < 4 ){
      logger().error("Not enough words to parse the packet!");
      return 1;
    }

    //Check SOP and EOP
    if( raw_words.front() != 0x13C ){
      logger().error("First word is NOT SOP (0x13C), but is 0x{:x}", raw_words.front());
      return 1;
    }
    if( raw_words.back() != 0x1DC ){
      logger().error("Last word is NOT EOP (0x1DC), but is 0x{:x}", raw_words.back());
      return 1;
    }

    //Get packet type
    int raw_type = (raw_words[1]>>4) & 0xF;
    if( packet_type_headers.find(raw_type) == packet_type_headers.end() ){
      logger().error("Error, packet type was parsed as {}, which is an invalid type.", raw_type);
      this->print_words(std::cout);
      this->type = TYP_UNKNOWN;
      return 1;
    }
    this->type = packet_type_headers[ raw_type ] ;

    logger().debug("Packet type is {}",
                    packet_type_names[this->type]);

    //Parse data for various packet types
    if (this->type == TYP_HCC_RR || this->type == TYP_HCC_HPR){
      return this->parse_data_HCC_read();
    }else if (this->type == TYP_ABC_RR || this->type == TYP_ABC_HPR){
      return this->parse_data_ABC_read();
    }else if(this->type == TYP_LP || this->type == TYP_PR){
      return this->parse_data_PRLP();
    }else if(this->type == TYP_ABC_TRANSP){
      return this->parse_data_ABC_transparent();
    }

    return 0;
  }//parse

  //Check if the raw words between two packets are identical (ordering matters!)
  bool compare_raw_words(StarChipPacket* compare_packet){
    if(this->n_words() != compare_packet->n_words())
      return false;

    for(unsigned int iW=0; iW < this->n_words(); ++iW){
      if( this->raw_words.at(iW) != compare_packet->raw_words.at(iW) )
        return false;
    }

    //Packet raw words are identical!
    return true;
  }

  //Check if parsed data between two packets is identical (ignores cluster ordering)
  bool compare(StarChipPacket* compare_packet){
    if( this->address      != compare_packet->address ||
        this->value        != compare_packet->value ||
        this->channel_abc  != compare_packet->channel_abc ||
        this->flag         != compare_packet->flag ||
        this->bcid         != compare_packet->bcid ||
        this->bcid_parity  != compare_packet->bcid_parity ||
        this->l0id         != compare_packet->l0id ||
        this->n_clusters() != compare_packet->n_clusters() )
    {
     return false;
    }

    for(unsigned int iC=0; iC < this->n_clusters(); ++iC){
      if( std::find(compare_packet->clusters.begin(), compare_packet->clusters.end(), this->clusters.at(iC) ) == compare_packet->clusters.end() )
        return false;
    }

    return true;
  }

  static void make_logger() {
    (void)logger();
  }
};

#endif
