#include <iostream>
#include <chrono>
#include <unistd.h>
#include "SpecController.h"
#include "SpecInfo.h"

int main(int argc, char *argv[]) {

    int specNum = 0;
    if (argc > 1) {
        specNum = std::stoi(argv[1]);
    }

	// create spec controller
	SpecCom mySpec(specNum);

	// create board info class
	SpecInfo info(&mySpec);

    std::cout << "Git hash: " << info.getCommitHash() << std::endl;
    std::cout << "Git date: " << info.getCommitDateString() << std::endl;
    std::cout << "Uptime: " << info.getUptime() << " s" << std::endl;
    std::cout << "Board-ID: 0x" << std::hex << info.getBoardId() << std::dec << " (" << info.getBoardString() << ")" << std::endl;
    std::cout << "TX channels: " << info.getNumTx() << std::endl;
    std::cout << "RX channels: " << info.getNumRx() << std::endl;

    return 0;
}
