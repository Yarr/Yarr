#include "SpecController.h"
#include "AD7995.h"
#include <iostream>
#include <unistd.h>
#include <chrono>

int main(void) {
    SpecController mySpec(0);

    std::cout << "Init" << std::endl;
    AD7995 adc(&mySpec);
    std::cout << "Set channels" << std::endl;
    adc.setActiveChannels(1, 0, 0, 0);
    std::cout << "Reading" << std::endl;
    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    while(1) {
        double value = adc.read();
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::seconds>(now-start).count() << " " << value << std::endl;
        sleep(1);
    }
    return 0;
}
