#include <SpecController.h>
#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define TX_ADDR (0x1 << 14)
#define RX_ADDR (0x2 << 14)

#define TX_CLK_PERIOD 8 // ns

#define TX_FIFO 0x0
#define TX_ENABLE 0x1
#define TX_UNDERRUN 0x2
#define TX_OVERRUN 0x3
#define TX_EMPTY 0x4
#define TRIG_EN 0x5
#define TRIG_DONE 0x6
#define TRIG_CONF 0x7
#define TRIG_FREQ 0x8
#define TRIG_TIME 0x9
#define TRIG_COUNT 0xB

#define RX_START_ADDR 0x0
#define RX_DATA_COUNT 0x1
#define RX_LOOPBACK 0x2
#define RX_DATA_RATE 0x3
#define RX_LOOP_FIFO 0x4

int main(void) {
    SpecController mySpec(0);
    std::string tmp;
    //std::cin >> tmp;
    const size_t size = 256;//256*5;

    uint32_t enable_mask = 0xFFFFFFFF;
    uint32_t disable_mask = 0x00000000;
    uint32_t *data = new uint32_t[size];
    memset(data, 0x55, size*4);
    uint32_t answer = 0;
#if 0 
    mySpec.readBlock(0x00004000, &answer, 1);
    std::cout << std::hex << answer << std::endl << std::dec;
    mySpec.writeBlock(0x00004001, &disable_mask, 1);
    
    mySpec.readBlock(0x00004001, &answer, 1);
    std::cout << std::hex << answer << std::endl << std::dec;
    
    mySpec.readBlock(0x00004001, &answer, 1);
    std::cout << "Enable: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004002, &answer, 1);
    std::cout << "Underrun: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004003, &answer, 1);
    std::cout << "Overrun: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004004, &answer, 1);
    std::cout << "Empty: 0x" << std::hex << answer << std::endl << std::dec;
    
    mySpec.writeBlock(0x00004001, &enable_mask, 1);
    for(int i=0 ; i<size; i++)
        mySpec.writeBlock(0x00004000, &data[i], 1);
    
    mySpec.readBlock(0x00004001, &answer, 1);
    std::cout << "Enable: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004002, &answer, 1);
    std::cout << "Underrun: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004003, &answer, 1);
    std::cout << "Overrun: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004004, &answer, 1);
    std::cout << "Empty: 0x" << std::hex << answer << std::endl << std::dec;
    
    
    mySpec.readBlock(0x00004001, &answer, 1);
    std::cout << "Enable: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004002, &answer, 1);
    std::cout << "Underrun: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004003, &answer, 1);
    std::cout << "Overrun: 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00004004, &answer, 1);
    std::cout << "Empty: 0x" << std::hex << answer << std::endl << std::dec;
#endif

    uint32_t disable = 0x0;
    uint32_t done = 0;
    //mySpec.writeBlock(TX_ADDR | TRIG_EN, &disable, 1);
    mySpec.readBlock(TX_ADDR | TRIG_DONE, &done, 1);
    mySpec.writeBlock(0x00004001, &enable_mask, 1);

    uint32_t conf = 0x1;
    uint32_t enable = 0x1;
    uint32_t frequency = 1/(double)(TX_CLK_PERIOD * 1e-9 *80000000.0); // 10MHz
    uint64_t time = 10.0/(double)(TX_CLK_PERIOD * 1e-9); // 10s
    uint32_t count = 340*256; // 10 trigger

    mySpec.writeBlock(TX_ADDR | TRIG_CONF, &conf, 1);
    mySpec.writeBlock(TX_ADDR | TRIG_FREQ, &frequency, 1);
    mySpec.writeBlock(TX_ADDR | TRIG_TIME, (uint32_t*)&time, 2);
    mySpec.writeBlock(TX_ADDR | TRIG_COUNT, &count, 1);

    mySpec.readBlock(TX_ADDR | TRIG_CONF, &answer, 1);
    std::cout << "Conf: " << answer << std::endl << std::dec;
    mySpec.readBlock(TX_ADDR | TRIG_FREQ, &answer, 1);
    std::cout << "Freq: " << 1e-3/(answer*(double)(TX_CLK_PERIOD * 1e-9)) << " kHz" << std::endl << std::dec;
    mySpec.readBlock(TX_ADDR | TRIG_TIME, (uint32_t*)&time, 2);
    std::cout << "Time: " << time*(double)(TX_CLK_PERIOD *1e-9) << " s"<< std::endl << std::dec;
    mySpec.readBlock(TX_ADDR | TRIG_COUNT, &answer, 1);
    std::cout << "Count: " << answer << std::endl << std::dec;
    
    timeval start, end;

    gettimeofday(&start, NULL);
    mySpec.writeBlock(TX_ADDR | TRIG_EN, &enable, 1);
    
    answer = 0;
    uint32_t data_cnt = 0;
    uint32_t addr = 0;
    done = 0;
    while(done == 0 || data_cnt == 0) {
        done = 0;
        addr = 0;
        data_cnt = 0;
        mySpec.readBlock(RX_ADDR | RX_DATA_RATE, &answer, 1);
        mySpec.readBlock(RX_ADDR | RX_START_ADDR, &addr, 1);
        mySpec.readBlock(RX_ADDR | RX_DATA_COUNT, &data_cnt, 1);
        if (data_cnt > 0) {
            mySpec.readBlock(TX_ADDR | TRIG_DONE, &done, 1);
            std::cout << "Rate = " << answer*4.0/1024.0/1024.0 << " MB/s" << std::endl;
            std::cout << "Start Adr = 0x" << std::hex << addr << std::endl << std::dec;
            std::cout << "Count = 0x" << std::hex << data_cnt << std::endl << std::dec;
            uint32_t *buf = new uint32_t[data_cnt];
            std::cout << "Starting DMA" << std::endl;
            if(mySpec.readDma(addr, buf, data_cnt)) {
                std::cout << "DMA FAILED!!!" << std::endl;
                return -1;
            }
            std::cout << "DMA done" << std::endl;
            //for (int i=0; i<data_cnt; i++)
            //    std::cout << "[" << i << "] 0x" << std::hex << buf[i] << std::dec << std::endl;
            delete buf;
            std::cout.flush();
        }         
    }
    gettimeofday(&end, NULL);
    usleep(500);
    mySpec.readBlock(RX_ADDR | RX_DATA_RATE, &answer, 1);
    mySpec.readBlock(RX_ADDR | RX_START_ADDR, &addr, 1);
    mySpec.readBlock(RX_ADDR | RX_DATA_COUNT, &data_cnt, 1);
    if (data_cnt > 0) {
        std::cout << "Rate = " << answer*4.0/1024.0/1024.0 << " MB/s" << std::endl;
        std::cout << "Start Adr = 0x" << std::hex << addr << std::endl << std::dec;
        std::cout << "Count = 0x" << std::hex << data_cnt << std::endl << std::dec;
        std::cout.flush();
        uint32_t *buf = new uint32_t[data_cnt];
        if(mySpec.readDma(addr, buf, data_cnt)) {
            std::cout << "DMA FAILED!!!" << std::endl;
            return -1;
        }
        delete buf;
    }         
    
    std::cout << "DONE" << std::endl;
    mySpec.writeBlock(TX_ADDR | TRIG_EN, &disable, 1);
    double stopwatch = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
    stopwatch += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
    std::cout << "Time: " << stopwatch << " ms" << std::endl;
    return 0;
}
