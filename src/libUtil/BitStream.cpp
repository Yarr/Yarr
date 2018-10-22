#include "BitStream.h"  
#include <sstream>
#include <iostream>

using namespace std;

BitStream::BitStream(){}

BitStream::~BitStream(){
  Clear();
}

BitStream::BitStream(string bitstring){
  FromString(bitstring);
}

void BitStream::Clear(){
  m_bits.clear();
  m_words.clear();
}
  
bool BitStream::Equals(const BitStream &b){
  if(GetNbits()!=b.GetNbits()){return false;}
  for(uint32_t i=0;i<m_bits.size();i++){
    if(m_bits[i]!=b.Get(i)){return false;}
  }
  return true;
}

BitStream & BitStream::Add(uint32_t size, uint32_t value){
  //Use the value from the LSB
  for(uint32_t i=0;i<size;i++){
	m_bits.push_back((value>>(size-i-1))&1);
  }
  return *this;
}

BitStream & BitStream::Set(uint32_t pos, bool value){
  if(m_bits.size()<=pos){m_bits.resize(pos+1,false);}
  m_bits[pos]=value;
  return *this;
}

BitStream & BitStream::Set(uint32_t pos, uint32_t size, uint32_t value){
  if(m_bits.size()<pos+size){m_bits.resize(pos+size,false);}
  for(uint32_t i=0;i<size;i++){
	m_bits[pos+i]=(value>>(size-i-1))&1;
  }
  return *this;
}

BitStream BitStream::Get(uint32_t pos, uint32_t size){
  BitStream ret;
  for(uint32_t i=0;i<size;i++){
    ret.Set(i,Get(pos+i));
  }
  ret.Pack();
  return ret;
}

bool BitStream::Get(uint32_t pos) const{
  if(m_bits.size()<pos){return 0;}
  return m_bits[pos];
}
  
uint32_t BitStream::GetWord(uint pos){
  if(m_words.size()<pos+1){return 0;}
  return m_words[pos];
}
  
uint32_t BitStream::GetSize(){
  return m_words.size();
}

uint32_t BitStream::GetNbits() const{
  return m_bits.size();
}

BitStream & BitStream::Pad(){
  uint32_t i0=0;
  //cout << "Find the first bit" << endl;
  for(uint32_t i=1; i<m_bits.size(); i++){
    if(m_bits[i]!=0){i0=i;break;}
  }
  //cout << "Copy the array forward" << endl;
  for(uint32_t i=0; i<m_bits.size()-i0; i++){
    m_bits[i]=m_bits[i+i0];
  }
  //cout << "Remove first " << i0 << " characters" << endl;
  for(uint32_t i=0; i<i0; i++){
    m_bits.pop_back();
  }
  return *this;
}

void BitStream::Pack(){
  uint32_t b=0;
  uint32_t c=0;
  for(int32_t i=m_bits.size()-1; i>=0; i--){
	if(m_bits[i]){b|=(1<<c);}
	//cout << "i=" << i << " w=" << (1<<c) << " " << "0x" << hex << b << dec << endl;
	c++;
	if((c%32==0 && c!=0) || i==0){m_words.insert(m_words.begin(),b);b=0;c=0;}
  }  
}

void BitStream::FromString(string s){
  FromBinString(s);
}

void BitStream::FromBinString(string s){
  for(uint32_t i=0;i<s.length();i++){
    Set(i,(s.at(i)=='1'?true:false));
  }
}

unsigned char hex2byte(const char* hex)
{
    unsigned short byte = 0;
    std::istringstream iss(hex);
    iss >> std::hex >> byte;
    return byte % 0x100;
}

void BitStream::FromHexString(string hexStr){
  for (unsigned i = 0; i<hexStr.length(); ++i) {
    string charStr = hexStr.substr(i,1);
    unsigned char hexVal = hex2byte(charStr.c_str());
    Add(4, static_cast<uint8_t>(hexVal));
  }
}

void BitStream::FromHex(string hexString){
  FromHexString(hexString);
}

void BitStream::ParseHex(string hexString){
  FromHexString(hexString);
}

string BitStream::ToString(){
  return ToBinString();
}

string BitStream::ToBinString(){
  ostringstream os;
  for(uint32_t i=0; i<m_bits.size(); i++){
	os << (m_bits[i]?"1":"0");
  }
  return os.str();
}

string BitStream::ToHexString(){
  const string shex[]={"0","1","2","3","4","5","6","7",
					   "8","9","A","B","C","D","E","F"};
  string os;
  uint32_t b=0;
  uint32_t c=0;
  for(int32_t i=m_bits.size()-1; i>=0; i--){
	if(m_bits[i]){b|=(1<<c);}
	//cout << "i=" << i << " b=" << (1<<c) << " " << "0x" << hex << b << dec << endl;
	c++;
	if((c%4==0 && c!=0) || i==0){os.insert(0,shex[b]);b=0;c=0;}
  }
  return os;

}




