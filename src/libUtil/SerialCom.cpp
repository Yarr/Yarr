#include "SerialCom.h"

SerialCom::SerialCom() {
    dev = 0;
    baudrate = B115200;
}

SerialCom::SerialCom(std::string deviceName) {
    dev = 0;
    baudrate = B115200;
    this->init(deviceName); 
}

SerialCom::~SerialCom() {
    if (dev)
        close(dev);
}

void SerialCom::init(std::string deviceName) {
    dev = open(deviceName.c_str(), O_RDWR | O_NOCTTY);
    this->config();
}

void SerialCom::config() {
    fcntl(dev, F_SETFL, 0);
    
    if (tcgetattr(dev, &tty))
        std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << " : Could not get tty attributes!" << std::endl;

    tty_old = tty;

    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    tty.c_cflag &= ~PARENB; // no parity
    tty.c_cflag &= ~CSTOPB; // one stop bit
    tty.c_cflag &= ~CRTSCTS; // no flow control
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; // 8 bits
    tty.c_cflag |= CREAD | CLOCAL;
    
    //tty.c_cc[VMIN]   =  0; 
    //tty.c_cc[VTIME]   =  0.5;

    cfmakeraw(&tty);
    tty.c_lflag |= ICANON; 
    
    tcflush(dev, TCIFLUSH);
    if (tcsetattr(dev, TCSANOW, &tty))
        std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << " : Could not set tty attributes!" << std::endl;
}

int SerialCom::write(char *buf, size_t length) {
    return ::write(dev, buf, length);
}

int SerialCom::read(char *buf, size_t length) {
    return ::read(dev, buf, length);
}

int SerialCom::write(std::string buf) {
    //std::cout << __PRETTY_FUNCTION__ << " : " << buf << std::endl;
    return ::write(dev, buf.c_str(), buf.size());
}

int SerialCom::read(std::string &buf) {
    char *tmp = new char[MAX_READ];
    unsigned n_read = ::read(dev, tmp, MAX_READ);
    buf = std::string(tmp, n_read);
    //std::cout << __PRETTY_FUNCTION__ << " : " << buf << std::endl;
    return n_read;
}

