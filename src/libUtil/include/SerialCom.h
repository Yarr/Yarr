#ifndef SERIALCOM_H
#define SERIALCOM_H

#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
class SerialCom {
    public:
        SerialCom();
        SerialCom(std::string deviceName);
        ~SerialCom();
    
        void init(std::string deviceName);
        void config();
        
        int write(char *buf, size_t length);
        int write(std::string buf);
        int read(char *buf, size_t length);
        int read(std::string &buf);
    
    private:

        const unsigned MAX_READ = 4096;
        int dev;

        speed_t baudrate;
        struct termios tty;
        struct termios tty_old;
};

#endif
