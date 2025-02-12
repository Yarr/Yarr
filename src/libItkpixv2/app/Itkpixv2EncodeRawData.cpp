#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <bitset>

#include "Itkpixv2Encoder.h"
#include "ItkpixEncoder.h"
#include "ItkpixLayout.h"
#include "EventData.h"
#include "Histo1d.h"
#include "Histo2d.h"

void usage(char* argv[])
{
    std::cout << "Usage: " << argv[0] << " [options] inputFile1 inputFile2 ..." << std::endl;
    std::cout << "    List of options:" << std::endl;
    std::cout << "        -o <output_dir> (default: ./encoded)" << std::endl;
    std::cout << "        -s <start_event> (default: 0)" << std::endl;
    std::cout << "        -e <end_event> (default: -1)" << std::endl;
    std::cout << "        -w <word_max> (default 255)" << std::endl;
    std::cout << "        -m <hit_max> (default 50)" << std::endl;
    std::cout << "        -i <bits_per_hit_max> (default 100)" << std::endl;
    std::cout << "        -k Toggle dumping of data words to binary file (default false)" << std::endl;
    std::cout << "        -p Toggle plotting of results (default false)" << std::endl;
    std::cout << "        -h Show this help message" << std::endl;
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

    std::string outputDir = "";
    int start = 0;
    int end = -1;
    int wordMax = 255;
    int hitMax = 50;
    int bphMax = 100;
    bool plot = false;
    bool dumpWords = false;

    // Parse CL
    int c;
    while ((c = getopt (argc, argv, "o:s:e:w:m:i:khp")) != -1)
    {
        switch (c)
        {
        case 'o':
            outputDir = std::string(optarg);
            break;
        case 's':
            start = std::stoi(optarg);
            break;
        case 'e':
            end = std::stoi(optarg);
            break;
        case 'w':
            wordMax = std::stoi(optarg);
            break;
        case 'm':
            hitMax = std::stoi(optarg);
            break;
        case 'k':
            dumpWords = true;
            break;
        case 'i':
            bphMax = std::stoi(optarg);
            break;
        case 'p':
            plot = true;
            break;
        case 'h':
            usage(argv);
            return -1;
        }
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
    // std::string path = "/Users/luclepot/Documents/ATLAS/pixels/test/data/011845_bella_exttrigger/0x20c52_data.raw";

    bool set_outputdir = (bool)(outputDir.size() == 0);
    for(int n = 0; n < inputFiles.size(); n++) {
        
        ItkpixLayout<short unsigned int> hitMap;
        Itkpixv2Encoder *k = new Itkpixv2Encoder();
        k->setEventsPerStream(1);

        std::cout << "Processing " << inputFiles[n] << std::endl;

        std::string base_filename = inputFiles[n].substr(inputFiles[n].find_last_of("/\\") + 1);
        std::string dirname = inputFiles[n].substr(0, inputFiles[n].find_last_of("/\\") + 1);
        
        if (set_outputdir) {
            outputDir = dirname;
        }

        std::string::size_type const p(base_filename.find_last_of('_'));
        std::string chipname = base_filename.substr(0, p);
        std::cout << "\tGot chip " << chipname << " with dirname " << dirname << "\n";

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

        Histo1d words("words", (float)wordMax + 1, -0.5, (float)wordMax + 0.5);
        words.setXaxisTitle("64-bit word count");
        words.setYaxisTitle("Count");
        
        Histo1d hits("hits", (float)hitMax + 1, -0.5, (float)hitMax + 0.5);
        hits.setXaxisTitle("Hit count");
        hits.setYaxisTitle("Count");
        

        Histo1d bitsPerHit("bitsPerHit", (float)bphMax  + 1, -0.5, (float)bphMax + 0.5);
        bitsPerHit.setXaxisTitle("Bits per hit, per event");
        bitsPerHit.setYaxisTitle("Count");
        
        Histo2d wordsVsHits(
            "wordsVsHits", 
            (float)wordMax + 1, -0.5, (float)wordMax + 0.5,
            (float)hitMax + 1, -0.5, (float)hitMax + 0.5
        );

        wordsVsHits.setXaxisTitle("64-bit word count");
        wordsVsHits.setYaxisTitle("Hit count");

        std::ifstream test(outputDir);
        if (!test) {
            std::string mkdirCmd = "mkdir -p " + outputDir;
            if (system(mkdirCmd.c_str()) < 0){
                std::cout << "#ERROR# Failed to create " << outputDir << " directory, not able to save data / plots!" << std::endl;
                continue;
            }
        }
        
        std::ofstream fout(outputDir + chipname + "_data.binary", std::ios::binary);

        while(inputFile) {
            if (n_events++ < start)
                continue;

            evo.fromFileBinary(inputFile);
            i++;
            hitMap.reset();
            for(int hit_n = 0; hit_n < evo.hits.size(); hit_n++) {
                // std::cout << "hit_n: " << hit_n << std::endl;
                if (evo.hits[hit_n].row < 1 || evo.hits[hit_n].col < 1 || evo.hits[hit_n].row >  384 || evo.hits[hit_n].col > 400) {
                    std::cout << "ERROR: got hit with zero row/col: " << evo.hits[hit_n].row << ", " << evo.hits[hit_n].col << ", " << evo.hits[hit_n].tot << std::endl;
                }
                else {
                    hitMap(evo.hits[hit_n].col - 1, evo.hits[hit_n].row - 1) = evo.hits[hit_n].tot;
                    n_hits++;
                }
            }
            // if(i == 395) {
            //     continue;
            //     // std::cout << evo.hits[hit_n].row << ", " << evo.hits[hit_n].col << ", " << evo.hits[hit_n].tot << std::endl;
            // }
            // k->setHitMap(hitMap);
            k->addToStream(hitMap, (uint8_t)(evo.tag & 0xFF));
            auto this_words = k->getWords().size();
            n_words += this_words;
            words.fill(this_words/2);
            hits.fill(evo.hits.size());
            wordsVsHits.fill(this_words/2, evo.hits.size());

            if(evo.hits.size() > 0) {
                float wph = 32*(((float)this_words) / (float)evo.hits.size());
                // if (wph > 3) {
                //     std::cout << evo.hits.size() << ", " << k->wordSize() << std::endl;
                // }
                bitsPerHit.fill(wph);
            }
            else {
                bitsPerHit.fill(0);
            }

            // word_counts.push_back(k->wordSize());
            if (i % 10000 == 0) {
                std::cout << "\t   Event " << i << ": hit size " << evo.hits.size() << ", word_count " << this_words << std::endl;
            }
            if (dumpWords) {
                auto w_elt = k->getWords();
                // std::ofstream fout(outputDir + chipname + "_data.binary", ios::binary);
                fout.write((char*)&w_elt[0], w_elt.size() * sizeof(w_elt));
            }
            k->getWords().clear();

            evo.hits.clear();
            evo.nHits = 0;
            
            if (n_events == end)
                break;

        }
        fout.close();
        inputFile.close();
        
        std::cout << "\tEncoded word size: " << n_words << ", with " << 32*n_words << " bits\n";
        std::cout << "\tHits: " << n_hits << ", Events: " << n_events << "\n";
        std::cout << "\tBits/Event: " << ((float)n_words*32)/((float)n_events) << std::endl;
        std::cout << "\tSaving results to " << outputDir << " with FE name " << chipname << std::endl;

        

        words.toFile(chipname, outputDir);
        hits.toFile(chipname, outputDir);
        bitsPerHit.toFile(chipname, outputDir);
        wordsVsHits.toFile(chipname, outputDir);

        if(plot){
            std::cout << "\tMaking plots..." << std::endl;

            words.plot(chipname, outputDir);  
            hits.plot(chipname, outputDir);
            bitsPerHit.plot(chipname, outputDir);
            wordsVsHits.plot(chipname, outputDir);
        }
        std::cout << "\n"; 
        
    }
    return 0;
}