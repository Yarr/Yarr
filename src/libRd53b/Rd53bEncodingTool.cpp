#include "Rd53bEncodingTool.h"

#include <bitset>

// Constructor with parameters:
Rd53bEncodingTool::Rd53bEncodingTool() : m_addresscompression(true),
                                         m_compression(true),
                                         m_suppressToT(false),
                                         m_eventsPerStream(1),
                                         m_suppressEndOfEvent(false),
                                         m_testEvent(0),
                                         m_streamTag(0),
                                         m_intTag(0),
                                         m_plainHitMap(false),
                                         m_debug(false)
{
  m_testStreamsStr.clear();
  m_chipMap = std::unique_ptr<Rd53bChipMap>(new Rd53bChipMap(400, 384, 8, 2));
  m_truthData = std::unique_ptr<FrontEndData>(new FrontEndData());
}

Rd53bEncodingTool::StatusCode Rd53bEncodingTool::saveDataStream()
{
  addOrphanBits(); // Add orphanBits

  // Split the stream strings into 63-bit blocks and prepend NS bit, making it a full 64-bit block
  for (auto stream : m_testStreamsStr)
  {
    //NS bits are not there yet, so all we have now is 63 bits of the actual data, hence the division by 63
    
    int nBlock = stream.length() / 63;

    #if use64
    m_outWords.reserve(m_outWords.capacity() + nBlock);
    #else
    //if outputing 32 bit words, we need to reserve 2 times the number of 64-bit encoded blocks
    m_outWords.reserve(m_outWords.capacity() + 2*nBlock);
    #endif

    for (int ib = 0; ib < nBlock; ib++)
    { 
      //convert 63 bits of the data from string to uint64_t
      uint64_t data   = std::stoull(stream.substr(ib*63, 63), nullptr, 2);
      //add the NS bit at the beginning
      uint64_t output = ((ib == 0 ? 0x1ULL : 0x0ULL) << 63) | data;//+ std::bitset<63>(stream.substr(ib * 63, 63)).to_ullong();
      //by now, we have a complete, 64-bit block including NS bit

      #if use64
      m_outWords.push_back(output);
      #else
      //split the 64 bits into two
      uint32_t word1 = output >> 32;
      uint32_t word2 = output & 0xFFFFFFFF;
      m_outWords.push_back(word1);
      m_outWords.push_back(word2);

      #endif
    }
  }

  return Rd53bEncodingTool::StatusCode::SUCCESS;
}

Rd53bEncodingTool::StatusCode Rd53bEncodingTool::initialize()
{
  return Rd53bEncodingTool::StatusCode::SUCCESS;
}

int Rd53bEncodingTool::getNumOfOrphanBits(int length, int size)
{
  //calculate the complement of length to be divisible by size
  return (size - length % size) % size;
}

std::string Rd53bEncodingTool::getNewTag()
{
  m_testEvent++;
  std::bitset<8> TagX_bitset;
  if (m_eventsPerStream > 1 && m_testEvent % m_eventsPerStream != 1)
  {
    m_intTag++;
    TagX_bitset |= m_intTag;
  }
  else
  {
    m_streamTag++;
    m_intTag = 0;
    TagX_bitset |= m_streamTag;
  }
  return TagX_bitset.to_string();
}

// Each time this function is called, a new event is created
Rd53bEncodingTool::StatusCode Rd53bEncodingTool::createStream(Rd53bChipMap &chipmap)
{
  //truth_hits.reserve(chipmap.getFiredPixels());
  //m_truthHits.reserve(m_truthHits.capacity() + chipmap.getFiredPixels());
  m_truthData->newEvent(0, 0, m_testEvent);

  std::string chipTag = getNewTag();

  // Need to start a new stream
  // test Event start from 1
  if (m_eventsPerStream > 1 && m_testEvent % m_eventsPerStream != 1)
  {
    chipTag = "111" + chipTag;
    m_testStreamsStr.back() += chipTag;
  }
  else
  {
    m_testStreamsStr.push_back(chipTag);
  }

  if (not chipmap.isFilled())
    chipmap.fillRegions();

  if (chipmap.getFiredPixels() == 0)
  {
    // once all this is done, you return
    return Rd53bEncodingTool::SUCCESS;
  }

  // if the chip map is not empty you need to loop on all the fired cores and store the information
  // about the ccol, qrow, bittree, tot
  for (int ccol = 0; ccol < chipmap.getCcols(); ccol++)
  {
    bool isfirst_ccol = true;
    int previous_qrow = -10; // use a default negative value
    int processed_hits = 0;
    for (int qrow = 0; qrow < chipmap.getQrows(); qrow++)
    {

      // -- STEP 1) Construct the data (bitmap+tot)
      // 1) get the bit map length
      // 1.1) if needed, skip the cores without hits
      // 2) get the tot for the selected core

      // 1) get the bit map length
      std::string bitmap_bits = m_plainHitMap ? chipmap.getPlainHitMap(ccol, qrow) : chipmap.getBitTreeString(ccol, qrow, m_compression);
      // 1.1) if needed, skip the cores without hits
      if (bitmap_bits == "")
        continue;

      std::vector<int> tots;

      // 2) get the tot for the selected core
      std::string tot_bits = chipmap.getToTBitsString(ccol, qrow, tots);
      if (m_suppressToT)
        tot_bits = "";

      int min_eta = ccol * chipmap.getCcolsRegion();
      int max_eta = (ccol + 1) * chipmap.getCcolsRegion() - 1;
      int min_phi = qrow * chipmap.getQrowsRegion();
      int max_phi = (qrow + 1) * chipmap.getQrowsRegion() - 1;

      int el = 0;
      bool is_first = true;
      for (int phi = min_phi; phi <= max_phi; phi++)
      {
        for (int eta = min_eta; eta <= max_eta; eta++)
        {
          if (tots.at(el) == 0)
          {
            el++;
            continue;
          }

          el++;          
          //hitCoordinate h;
          //h.event = m_testEvent;
          //h.col = eta;
          //h.row = phi;
          //h.tot = tots.at(el-1);
          //m_truthHits.push_back(h);
          FrontEndHit feHit;
          feHit.col = eta + 1;
          feHit.row = phi + 1;
          feHit.tot = tots.at(el - 1) - 1;
          m_truthData->curEvent->addHit(feHit);
          processed_hits++;
        }
      }

      // -- STEP 2) Add the tags
      // now you need the ccol, qrow tags + islast_isneighbour:
      // 1) add 2 bits for islast and isneighbour bits
      // 2) if it is the first ccol you need to add the ccol tag
      // 3) add the qrow if the current qrow is not neighbour of previous qrow
      // 4) update the stream

      // 1) add 2 bits for islast and isneighbour bits
      if (m_addresscompression)
      {
        // 2) if it is the first ccol you need to add the ccol tag
        if (isfirst_ccol)
        {
          isfirst_ccol = false;
          std::bitset<6> ccol_bits_string;
          ccol_bits_string |= ccol + 1;
          m_testStreamsStr.back() += ccol_bits_string.to_string();
        }

        std::string islast_isneighbour_qrow_bits = "";
        if (processed_hits == chipmap.getFiredPixelsInCcol(ccol))
          islast_isneighbour_qrow_bits += "1";
        else
        {
          islast_isneighbour_qrow_bits += "0";
        }

        // 3) add the qrow if the current qrow is not neighbour of previous qrow
        if (previous_qrow != (qrow - 1))
        {
          islast_isneighbour_qrow_bits += "0";
          std::bitset<8> qrow_bits_string;
          qrow_bits_string |= qrow; // Qrow index should start from 0
          islast_isneighbour_qrow_bits += qrow_bits_string.to_string();
        }
        else
          islast_isneighbour_qrow_bits += "1";

        // update the previous_row to the current
        previous_qrow = qrow;

        m_testStreamsStr.back() += islast_isneighbour_qrow_bits;
      }
      else
      {
        std::bitset<6> ccol_bits_string;
        ccol_bits_string |= ccol + 1;
        m_testStreamsStr.back() += ccol_bits_string.to_string();

        std::bitset<8> qrow_bits_string;
        qrow_bits_string |= qrow;
        m_testStreamsStr.back() += qrow_bits_string.to_string();
      }

      m_testStreamsStr.back() += (bitmap_bits + tot_bits);
    }
  }

  return StatusCode::SUCCESS;
}

int Rd53bEncodingTool::addOrphanBits()
{
  // Split the stream strings into 64-bit blocks
  int nBlock = 0;
  for (auto& stream : m_testStreamsStr)
  {
    int length = stream.length();
    int orphan_bits = getNumOfOrphanBits(length);
    if (!m_suppressEndOfEvent && orphan_bits < 6)
      orphan_bits += 63;
    if (orphan_bits > 0)
    {
      for (int orp = 0; orp < orphan_bits; orp++)
        stream += "0";
    }

    nBlock += stream.length() / 63;
  }

  return nBlock;
}

void Rd53bEncodingTool::generate(int totEvents, double occupancy, int clusterSize, int orientation){
  //This is a wrapper to call generation/encoding in correct order
  //to produce the hits vector and vector of the encoded words for
  //arbitrary number of events/events per stream. Number of events
  //per stream is assumed to be fixed by the time generate() is called

  for (int ievt = 0; ievt < totEvents; ievt++){
    //generate the hit map
    //m_chipMap.reset(new Rd53bChipMap(400, 384, 8, 2));
    m_chipMap->generateRndmEvent(occupancy, clusterSize, orientation);
    //encode the event into the stream string
    createStream(*m_chipMap);
    m_chipMap->reset();
  }
  
  //add orphan bits, break each stream string into blocks, pre-pend NS bit
  //and store in the words vector
  saveDataStream();

}

