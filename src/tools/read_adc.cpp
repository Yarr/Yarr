#include "SpecCom.h"
#include "AD7995.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iomanip>

int main(int argc, char* argv[]) {
    SpecCom mySpec(0);

    std::cout << "Init" << std::endl;
    AD7995 adc(&mySpec);
    std::cout << "Set channels" << std::endl;
    adc.setActiveChannels(1, 0, 0, 0);
    
    std::fstream ofile(argv[1], std::ios::out);
    if (!ofile) {
        std::cout << " Coudln't open file: " << argv[1] << std::endl;
        return 1;
    }

    std::cout << "Reading" << std::endl;

    long double timestamp = 0;
    while(1) {
        adc.read();
        double value = adc.getValue(0);
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1.0e6;
        std::cout << std::setprecision(15) << timestamp << " " << value << std::endl;
        ofile << std::setprecision(15) << timestamp << " " << value << std::endl;
        sleep(1);
    }
    return 0;
}
