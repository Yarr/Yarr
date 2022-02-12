#include <bitset>
#include <getopt.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>

#include "LUT_PlainHMapToColRow.h"
#include "RawData.h"

#define BLOCKSIZE 64
#define HALFBLOCKSIZE 32
#define BINARYTREE_DEPTH 4

static constexpr uint8_t _LUT_BinaryTreeMaskSize[3] = {4, 2, 1};
static constexpr uint8_t _LUT_BinaryTreeMask[3][8] = {
    {0xF0, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00},
    {0xCF, 0x00, 0x3F, 0x00, 0xFC, 0x00, 0xF3, 0x00},
    {0xBF, 0x7F, 0xEF, 0xDF, 0xFB, 0xF7, 0xFE, 0xFD},
};

struct option longopts[] = {
    {"input", required_argument, NULL, 'i'},
    {"output", required_argument, NULL, 'o'},
    {"ne", required_argument, NULL, 'n'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}};

std::vector<uint32_t> _buffer;
int _blockIdx = 0;
int _bitIdx = 0;
uint32_t *_data = NULL;
std::unique_ptr<RawData> _curIn;

void printHelp(const std::string exe)
{
    std::cout << "Usage: " << exe << " [options]" << std::endl;
    std::cout << "Allowed options:" << std::endl;
    std::cout << " -i [ --input ] arg                 Input bit stream file (required)" << std::endl;
    std::cout << " -o [ --output ] arg                Output file name" << std::endl;
    std::cout << " -n [ --ne ] arg                    Number of events per stream (if equals to 1, will skip the internal tag check)" << std::endl;
    std::cout << " -h [ --help ]                      Produce help message" << std::endl;
}

class Rd53bDecodeHelper
{
public:
    Rd53bDecodeHelper() { _remain = ""; }
    ~Rd53bDecodeHelper()
    {
        inputStream.clear();
        outputStream.clear();
        auxStream1.clear();
    }
    void fill(std::string input, const std::string output, const std::string name, const bool startNewLine = false);
    void save(const std::string outputFileName);
    void block(const uint64_t blockStream);
    void remain(const std::string input);

private:
    std::string decorate(const std::string str, const unsigned length, const char separator);
    std::vector<std::stringstream *> inputStream;
    std::vector<std::stringstream *> outputStream;
    std::vector<std::stringstream *> auxStream1;
    std::vector<std::stringstream *> auxStream2;
    std::vector<std::stringstream *> auxStream3;
    std::string _remain;
};

void Rd53bDecodeHelper::fill(std::string input, const std::string output, const std::string name, const bool startNewLine)
{
    if (startNewLine)
    {
        inputStream.push_back(new std::stringstream);
        outputStream.push_back(new std::stringstream);
        auxStream1.push_back(new std::stringstream);
        auxStream2.push_back(new std::stringstream);
    }
    if (inputStream.back()->str().length() == 3 && inputStream.size() > 1 && _remain != "")
    {
        input = "(" + _remain + ")" + input.substr(_remain.length());
        _remain = "";
    }
    const unsigned maxLength = std::max(std::max(input.length(), output.length()), name.length()) + 1;
    *inputStream.back() << decorate(input, maxLength, ' ');
    *outputStream.back() << decorate(output, maxLength, ' ');
    *auxStream1.back() << decorate(name, maxLength, ' ');
    *auxStream2.back() << decorate("", maxLength, '_');
}

std::string Rd53bDecodeHelper::decorate(const std::string str, const unsigned length, const char separator)
{
    unsigned left = (length - str.length()) / 2;
    unsigned right = length - str.length() - left;
    std::string output = "";
    for (int i = 0; i < left; i++)
        output += separator;
    output += str;
    for (int i = 0; i < right; i++)
        output += separator;
    return output;
}

void Rd53bDecodeHelper::save(const std::string outputFileName)
{
    std::ofstream fout(outputFileName);
    for (int is = 0; is < inputStream.size(); is++)
    {
        std::string stream = (*inputStream[is]).str();
        fout << (*auxStream3[is]).str() << std::endl;
        fout << (*inputStream[is]).str() << std::endl;
        fout << (*auxStream1[is]).str() << std::endl;
        fout << (*outputStream[is]).str() << std::endl;
        fout << (*auxStream2[is]).str() << std::endl;
        fout << std::endl;
    }
    fout.close();
}

void Rd53bDecodeHelper::block(const uint64_t blockStream)
{
    auxStream3.push_back(new std::stringstream);
    *auxStream3.back() << std::bitset<64>(blockStream);
}

void Rd53bDecodeHelper::remain(const std::string input)
{
    fill(input, "", "Remain");
    _remain = input;
}

void startNewStream(Rd53bDecodeHelper *h, unsigned &nEvents)
{
    /* Start a new stream */
    _data = &_curIn->buf[2 * _blockIdx];

    uint8_t tag = (_data[0] >> 23) & 0xFF;
    _blockIdx++; // Increase block index
    _bitIdx = 9; // Reset bit index = NS + tag

    h->block((uint64_t(_data[0]) << 32) | _data[1]);
    h->fill(std::bitset<1>(_data[0] >> 31).to_string(), "", "NS", true);
    h->fill(std::bitset<8>(tag).to_string(), std::to_string(tag), "Tag");
    nEvents++;
}

uint64_t retrieve(Rd53bDecodeHelper *h, const unsigned length, const bool checkEOS = false)
{
    if (length == 0)
        return 0;
    if (checkEOS && (_data[0] >> 31) && _bitIdx == 1)
    {					// Corner case where end of event mark (0000000) is suppressed and there is no orphan bits
        _blockIdx--; // Roll back block index by 1. A new stream will start in the next loop iteration
        return 0;
    }
    uint64_t variable = 0;

    if (_bitIdx + length <= BLOCKSIZE)
    { // Need to read in next block
        variable = (((_bitIdx + length) <= HALFBLOCKSIZE) || (_bitIdx >= HALFBLOCKSIZE)) ? ((_data[_bitIdx / HALFBLOCKSIZE] & (0xFFFFFFFFUL >> (_bitIdx - ((_bitIdx >> 5) << 5)))) >> ((((_bitIdx >> 5) + 1) << 5) - length - _bitIdx))
                                                                                               : (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << (length + _bitIdx - HALFBLOCKSIZE)) | (_data[1] >> (BLOCKSIZE - length - _bitIdx)));
        _bitIdx += length; // Move bit index
    }
    else
    {
        if (checkEOS && (_data[2] >> 31))
        { // Check end of stream
            return 0;
        }

        // If the actual length of curIn->buf is curIn->words + 2, the following line can be removed.
        // This can be easily done by filling in 0
        variable = (((_bitIdx < HALFBLOCKSIZE) ? (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << HALFBLOCKSIZE) | _data[1])
                                                  : (_bitIdx == BLOCKSIZE ? 0
                                                                             : (_data[1] & (0xFFFFFFFFUL >> (_bitIdx - HALFBLOCKSIZE)))))
                    << (length + _bitIdx - BLOCKSIZE)) |
                   (((_bitIdx + length) < (HALFBLOCKSIZE + BLOCKSIZE)) ? ((_data[2] & 0x7FFFFFFFUL) >> (0x5F & ~(_bitIdx + length)))
                                                                          : (((_data[2] & 0x7FFFFFFFUL) << (_bitIdx + length - 0x5F)) | (_data[3] >> (0x7F & ~(length + _bitIdx)))));

        if (_bitIdx < BLOCKSIZE)
            h->remain((std::bitset<64>((uint64_t(_data[0]) << 32) | _data[1]).to_string()).substr(_bitIdx));

        _blockIdx++; // Increase block index
        _data = &_data[2];
        _bitIdx -= (63 - length); // Reset bit index. Since we always read the NS bit the index should always start from 1
        h->fill(std::bitset<1>(_data[0] >> 31).to_string(), "", "NS", true);
        h->block((uint64_t(_data[0]) << 32) | _data[1]);
    }

    return variable;
}

void rollBack(const unsigned length)
{ // Roll back bit index
    if (length == 0)
        return;
    if (_bitIdx >= (length + 1))
        _bitIdx -= length; // Keep in mind there is one extra bit from NS
    else
    { // Across block, roll back by length
        _bitIdx += (63 & ~length);
        _data = &_curIn->buf[2 * (--_blockIdx - 1)];
    }
}

uint8_t getBitPair(Rd53bDecodeHelper *h)
{
    // if(_debug >= 3) std::cout << __PRETTY_FUNCTION__ << std::endl;
    const uint8_t bitpair = retrieve(h, 2);
    // There is only 11 and 10, and 01 is replaced with 0
    // Whenever 00 or 01 show up, replace the first 0 with 01, and move 1-bit back
    if (bitpair == 0x0 || bitpair == 0x1)
    {
        rollBack(1); // Moving bit index back by 1
        h->fill("0", "0(1)", "");
        return 0x1;
    }
    else
    {
        h->fill(std::bitset<2>(bitpair).to_string(), std::bitset<2>(bitpair).to_string(), "");
        return bitpair;
    }
}

uint16_t readRow(Rd53bDecodeHelper *h)
{
    uint16_t hitmap = 0XFF; // Start from all 1. Pixels not fired will be masked by 0
    uint8_t nRead[BINARYTREE_DEPTH] = {1, 0, 0, 0};
    uint8_t nShift[BINARYTREE_DEPTH][16] = {{0}};
    for (uint8_t lv = 0; lv < BINARYTREE_DEPTH - 1; lv++)
    { // For each row there
        // std::cout << "Depth = " << int(lv) << std::endl;
        for (uint8_t ir = 0; ir < nRead[lv]; ir++)
        {
            uint8_t bitpair = getBitPair(h);
            // if(lv == BINARYTREE_DEPTH - 1) continue;
            switch (bitpair)
            {
            case 0x1: // 01
                hitmap &= _LUT_BinaryTreeMask[lv][nShift[lv][ir]];
                nShift[lv + 1][nRead[lv + 1]++] = nShift[lv][ir];
                break;
            case 0x2: // 10
                hitmap &= _LUT_BinaryTreeMask[lv][nShift[lv][ir] + _LUT_BinaryTreeMaskSize[lv]];
                nShift[lv + 1][nRead[lv + 1]++] = nShift[lv][ir] + _LUT_BinaryTreeMaskSize[lv];
                break;
            case 0x3: // 11
                hitmap &= (_LUT_BinaryTreeMask[lv][nShift[lv][ir]] | _LUT_BinaryTreeMask[lv][nShift[lv][ir] + _LUT_BinaryTreeMaskSize[lv]]);
                nShift[lv + 1][nRead[lv + 1]++] = nShift[lv][ir] + _LUT_BinaryTreeMaskSize[lv];
                nShift[lv + 1][nRead[lv + 1]++] = nShift[lv][ir];
                break;
            }
        }
    }
    return hitmap;
}

int readInData(std::string inputStreamFileName, int nStream = -1)
{
    std::cout << "Reading encoded data...";
    uint32_t address = 0xFFFFFFFF;
    unsigned words = 0;
    std::ifstream fstream(inputStreamFileName);
    std::string line;
    int streamCnt = 0;
    while ((std::getline(fstream, line)))
    {
        uint32_t data1 = std::bitset<32>(line.substr(0, 32)).to_ulong();
        uint32_t data2 = std::bitset<32>(line.substr(32, 32)).to_ulong();
        _buffer.push_back(data1);
        _buffer.push_back(data2);
        if (((data1 >> 31) & 0x1))
            streamCnt++;
        if (nStream > 0 && streamCnt > nStream)
            break;
        words += 2;
    
    }
    fstream.close();
    _buffer.push_back(0);
    _buffer.push_back(0);

    _curIn.reset(new RawData(address, &_buffer[0], words));
    std::cout << "Done." << std::endl;
    return words;
}

int main(int argc, char **argv)
{
    std::string inputFileName = "";
    std::string outputFileName = "decoded.txt";
    bool compressed = true;
    int oc;
    while ((oc = getopt_long(argc, argv, ":i:o:c:h", longopts, NULL)) != -1)
    {
        switch (oc)
        {
        case 'i':
            inputFileName = optarg;
            break;
        case 'o':
            outputFileName = optarg;
            std::cout << "Set output file name to: " << outputFileName << std::endl;
            break;
        case 'c':
            compressed = atoi(optarg);
            std::cout << "Set hit map compression to: " << compressed << std::endl;
            break;
        case 'h':
            printHelp(argv[0]);
            return 0;
        case ':': /* missing option argument */
            fprintf(stderr, "%s: option `-%c' requires an argument\n",
                    argv[0], optopt);
            printHelp(argv[0]);
            return 0;
        case '?':
        default:
            fprintf(stderr, "%s: option `-%c' is invalid: ignored\n",
                    argv[0], optopt);
            printHelp(argv[0]);
            return 0;
        }
    }

    if (inputFileName == "")
    {
        std::cerr << "Input file not provided" << std::endl;
        return 0;
    }

    Rd53bDecodeHelper *h = new Rd53bDecodeHelper();

    readInData(inputFileName);
    const unsigned blocks = _curIn->words / 2;
    uint8_t qrow = 0, ccol = 0;
    uint8_t islast = 0, isneighbor = 0;

    unsigned nHits = 0, nEvents = 0;
    startNewStream(h, nEvents);
    while (_blockIdx <= blocks)
    {
        // Start from getting core column
        ccol = retrieve(h, 6, true);
        // End of stream marked with 000000 in current stream
        if (ccol == 0)
        {
            h->fill(std::bitset<6>(ccol).to_string(), "", "End of stream");
            if (_blockIdx >= blocks)
                break; // End of data processing
            startNewStream(h, nEvents);
            continue;
        }
        else if (ccol >= 0x38)
        { // Internal tag
            const uint16_t intTag = (ccol << 5) | retrieve(h, 5);
            h->fill(std::bitset<11>(intTag).to_string(), "", "Int tag");
            nEvents++;
            continue;
        }
        h->fill(std::bitset<6>(ccol).to_string(), std::to_string(ccol), "ccol");
        // Loop over all the hits
        do
        {
            if (_blockIdx > blocks)
                break; // End of data processing
            islast = retrieve(h, 1);
            isneighbor = retrieve(h, 1);
            h->fill(std::bitset<1>(islast).to_string(), "", "islast");
            h->fill(std::bitset<1>(isneighbor).to_string(), "", "isnext");

            if (isneighbor)
                ++qrow;
            else
            {
                qrow = retrieve(h, 8);
                h->fill(std::bitset<8>(qrow).to_string(), std::to_string(qrow), "qrow");
            }

            uint16_t hitmap = 0x0;
            if (compressed){
                // ++++++++++++++++++++++++ Brute force approach ++++++++++++++++++++++++
                uint8_t root = getBitPair(h);
                if (root & 0x2)
                { // Read the first row
                    hitmap = (readRow(h) & 0x00FF);
                }
                if (root & 0x1)
                { // Read the second row
                    hitmap |= (readRow(h) << 8);
                }
                h->fill("", std::bitset<16>(hitmap).to_string(), "Hit map");
            }
            else
            {
                hitmap = retrieve(h, 16);
                h->fill(std::bitset<16>(hitmap).to_string(), "", "Hit map");
            }

            // Precision ToT data
            if (qrow == 196) {
                for (int ibus = 0; ibus < 4; ibus++) {
                    // uint16_t PToT[3] = {0, 0, 0}, PToA[2] = {0, 0};
                    uint8_t hitsub = (hitmap >> (ibus << 2)) & 0xF;
                    if (hitsub) {
                        uint16_t ptot_ptoa_buf = 0xFFFF;
                        for(int iread = 0; iread < 4; iread++){
                            if ((hitsub >> iread) & 0x1){
                                uint8_t fourbit = retrieve(h, 4);
                                ptot_ptoa_buf &= ~((~fourbit & 0xF) << (iread << 2));
                                h->fill(std::bitset<4>(fourbit).to_string(), "", "");
                            }
                            else
                                h->fill("(1111)", "", "");
                        }

                        uint16_t PToT = ptot_ptoa_buf & 0x7FF;
                        uint8_t PToA = ptot_ptoa_buf >> 11;
                        h->fill("", std::to_string(PToT), "PToT" + std::to_string(ibus));
                        h->fill("", std::to_string(PToA), "PToA" + std::to_string(ibus));
                    }
                }
            }
            else{
                nHits += RD53BDecoding::_LUT_PlainHMap_To_ColRow_ArrSize[hitmap];
                for (int ihit = 0; ihit < RD53BDecoding::_LUT_PlainHMap_To_ColRow_ArrSize[hitmap]; ++ihit)
                {
                    uint8_t pix_tot = retrieve(h, 4);
                    h->fill(std::bitset<4>(pix_tot).to_string(), std::to_string(pix_tot), "ToT" + std::to_string(ihit));
                }
            }
        } while (!islast);
    }

    std::cout << "Output decoded file saved in " << outputFileName << std::endl;
    std::cout << "Number of events processed: " << nEvents
          << ", number of hits processed: " << nHits
          << std::endl;
    h->save(outputFileName);
}
