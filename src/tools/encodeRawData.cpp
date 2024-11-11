#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <bitset>

#include "Itkpixv2Encoder.h"
#include "ItkpixEncoder.h"
#include "EventData.h"
#include "Histo1d.h"
#include "Histo2d.h"

void usage(char* argv[])
{
    std::cout << "Usage: " << argv[0] << " [options] inputFile1 inputFile2 ..." << std::endl;
    std::cout << "    List of options:" << std::endl;
    std::cout << "        -o output_dir (default: ./encoded)" << std::endl;
    std::cout << "        -p prefix (default: '')" << std::endl;
    std::cout << "        -s <start_event> (default: 0)" << std::endl;
    std::cout << "        -e <end_event> (default: -1)" << std::endl;
    std::cout << "        -w <word_max> (default 255)" << std::endl;
    std::cout << "        -h <hit_max> (default 50)" << std::endl;
    exit(1);
}

bool fileExists(const std::string& filename)
{
    std::ifstream ifile(filename);
    return ifile.good();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage(argv);
        return -1;
    }

    std::string outputDir = "offline/";
    int start = 0;
    int end = -1;
    int wordMax = 255;
    int hitMax = 50;
    std::string prefix = "";
    // Parse CL
    int c;
    while ((c = getopt (argc, argv, "o:s:e:p:w:h:")) != -1)
    {
        switch (c)
        {
        case 'o':
            outputDir = std::string(optarg) + std::string("/");
            break;
        case 's':
            start = std::stoi(optarg);
            break;
        case 'e':
            end = std::stoi(optarg);
            break;
        case 'p':
            prefix = std::string(optarg);
            break;
        case 'w':
            wordMax = std::stoi(optarg);
            break;
        case 'h':
            hitMax = std::stoi(optarg);
            break;
        }
    }

    std::string mkdirCmd = "mkdir -p " + outputDir;
    if (system(mkdirCmd.c_str()) < 0){
        std::cout << "#ERROR# Failed to create " << outputDir << " directory, not able to save plots!" << std::endl;
    }

    std::string inputFile;
    std::vector<std::string> inputFiles;
    if (optind < argc)
    {
        while (optind < argc)
        {
            inputFile = argv[optind++];

            if (! fileExists(inputFile)){
                std::cout << "#ERROR# Input file \"" << inputFile << "\" does not exist or cannot be opened" << std::endl;
                return -1;
            }

            inputFiles.push_back(inputFile);
        }
    }

    if (inputFiles.size() == 0)
    {
        usage(argv);
    }

    std::vector<std::vector<uint16_t> > hitMap;
    hitMap.resize(400);
    for(int j = 0; j < 400; j++)
        for(int i = 0; i < 384; i++)
            hitMap[j].push_back(0);

    std::cout << "Hitmap size: " << hitMap.size() << " x " << hitMap[0].size() << std::endl;
    // std::string path = "/Users/luclepot/Documents/ATLAS/pixels/test/data/011845_bella_exttrigger/0x20c52_data.raw";

    for(int n = 0; n < inputFiles.size(); n++) {
        
        Itkpixv2Encoder *k = new Itkpixv2Encoder();
        k->setEventsPerStream(1);

        std::string path = std::string(argv[n]);
        std::cout << "Opening file: " << inputFiles[n] << std::endl;
        std::fstream inputFile(inputFiles[n], std::fstream::in | std::fstream::binary);
        if(!inputFile.good()) {
            std::cout << "FAILED!" << std::endl;
            continue;
        }
        
        FrontEndEvent evo;
        int i = 0;
        int n_hits = 0;
        int n_events = 0;
        // std::vector<uint8_t> word_counts;
        // word_counts.reserve(100000);
        int n_words = 0;
        int wphMax = wordMax/hitMax + 1;
        Histo1d words("words", (float)wordMax + 1, -0.5, (float)wordMax + 0.5);
        words.setXaxisTitle("64-bit word count");
        words.setYaxisTitle("Count");
        
        Histo1d hits("hits", (float)hitMax + 1, -0.5, (float)hitMax + 0.5);
        hits.setXaxisTitle("Hit count");
        hits.setYaxisTitle("Count");
        
        Histo1d wordsPerHit("wordsPerHit", (float)wphMax + 1, -0.05, (float)wphMax + 0.5);
        wordsPerHit.setXaxisTitle("64 bit words / hits, per event");
        wordsPerHit.setYaxisTitle("Count");
        
        Histo2d wordsVsHits(
            "wordsVsHits", 
            (float)wordMax + 1, -0.5, (float)wordMax + 0.5,
            (float)hitMax + 1, -0.5, (float)hitMax + 0.5
        );

        wordsVsHits.setXaxisTitle("64-bit word count");
        wordsVsHits.setYaxisTitle("Hit count");

        while(inputFile) {
            if (n_events++ < start)
                continue;

            evo.fromFileBinary(inputFile);
            i++;
            
            for(int col = 0; col < hitMap.size(); col++) {
                for(int row = 0; row < hitMap[col].size(); row++) {
                    hitMap[row][col] = 0;
                }
            }

            for(int hit_n = 0; hit_n < evo.hits.size(); hit_n++) {
                // std::cout << "hit_n: " << hit_n << std::endl;
                
                hitMap[evo.hits[hit_n].row - 1][evo.hits[hit_n].col - 1] = evo.hits[hit_n].tot;
                n_hits++;
            }
            // if(i == 395) {
            //     continue;
            //     // std::cout << evo.hits[hit_n].row << ", " << evo.hits[hit_n].col << ", " << evo.hits[hit_n].tot << std::endl;
            // }
            k->addToStream(hitMap, true, false);
            n_words += k->wordSize();
            words.fill(k->wordSize()/2);
            hits.fill(evo.hits.size());
            wordsVsHits.fill(k->wordSize()/2, evo.hits.size());

            if(evo.hits.size() > 0) {
                float wph = (((float)k->wordSize()/2) / (float)evo.hits.size());
                // if (wph > 3) {
                //     std::cout << evo.hits.size() << ", " << k->wordSize() << std::endl;
                // }
                wordsPerHit.fill(wph);
            }
            else {
                wordsPerHit.fill(0);
            }

            // word_counts.push_back(k->wordSize());
            if (i % 10000 == 0) {
                std::cout << "   Event " << i << ": hit size " << evo.hits.size() << ", word_count " << k->wordSize() << std::endl;
            }
            k->flushWords();

            evo.hits.clear();
            evo.nHits = 0;
            
            if (n_events == end)
                break;
        }
        inputFile.close();

        std::cout << "Got to encoding part.." << std::endl;
        
        // int n_words = 0;
        // for(int i = 0; i < word_counts.size(); i++) {
        //     n_words += word_counts[i];
        // }
        
        
        std::cout << "test main\n";
        std::cout << "Encoded word size: " << n_words << ", with " << 32*n_words << "bits\n";
        std::cout << "Hits: " << n_hits << ", Events: " << n_events << "\n";
        std::cout << "Bits/Event: " << ((float)n_words*32)/((float)n_events) << std::endl;

        words.plot(prefix, outputDir);
        words.toFile(outputDir + prefix);

        hits.plot(prefix, outputDir);
        hits.toFile(outputDir + prefix);

        wordsPerHit.plot(prefix, outputDir);
        wordsPerHit.toFile(outputDir + prefix);

        wordsVsHits.plot(prefix, outputDir);
        wordsVsHits.toFile(outputDir + prefix);
    }
    return 0;
}