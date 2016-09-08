#include <BitFile.h>

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <string>

namespace BitFile {
    int checkFile(std::fstream &file) {
        if (!file) return 1;
        char key;
        uint8_t c_length[2];
        uint16_t length;

        file.seekg(0, std::ios::beg);

        // Field 1
        file.read((char*)c_length, 2);
        length = c_length[1];
        length += c_length[0] << 8;
        if (length != 9) {
            std::cerr << __PRETTY_FUNCTION__ << " -> Wrong header length!" << std::endl;
            return 1;
        }

        char *field1 = new char[length];
        file.read(field1, length);

        // Field 2
        file.read((char*)c_length, 2);
        length = c_length[1];
        length += c_length[0] << 8;
        char *field2 = new char[length];
        file.read(field2, length);

        // Field 3
        file.read((char*)c_length, 2);
        length = c_length[1];
        length += c_length[0] << 8;
        char *field3 = new char[length];
        file.read(field3, length);

        // Field 4
        file.read(&key, 1);
        file.read((char*)c_length, 2);
        length = c_length[1];
        length += c_length[0] << 8;
        char *field4 = new char[length];
        file.read(field4, length);

        // Field 5
        file.read(&key, 1);
        file.read((char*)c_length, 2);
        length = c_length[1];
        length += c_length[0] << 8;
        char *field5 = new char[length];
        file.read(field5, length);

        // Field 6
        file.read(&key, 1);
        file.read((char*)c_length, 2);
        length = c_length[1];
        length += c_length[0] << 8;
        char *field6 = new char[length];
        file.read(field6, length);

        // Field 7
        file.read(&key, 1);
        uint8_t c_data_length[4];
        file.read((char*)c_data_length, 4);
        uint32_t data_length = 0;
        data_length = c_data_length[3];
        data_length += c_data_length[2] << 8;
        data_length += c_data_length[1] << 16;
        data_length += c_data_length[0] << 24;

        // Remaining file size
        size_t beg, end, size;
        beg = file.tellg();
        file.seekg(0, std::ios::end);
        end = file.tellg();
        size = end - beg;   
        file.seekg(0, std::ios::beg);

        std::cout << "=========================================" << std::endl;
        std::cout << "File info:" << std::endl;
        std::cout << " Design Name: " << field3 << std::endl;
        std::cout << " Device:      " << field4 << std::endl;
        std::cout << " Timestamp:   " << field5 << " " << field6 << std::endl;
        std::cout << " Data size:   " << data_length << std::endl;
        std::cout << "=========================================" << std::endl;

        std::string device(field4, 13);
        if (device != "6slx45tfgg484") {
            std::cerr << __PRETTY_FUNCTION__ << " -> Wrong device type!" << std::endl;
            return 1;
        }

        if (data_length-size != 0) {
            std::cerr << __PRETTY_FUNCTION__ << " -> Real size and size don't match!" << std::endl;
            return 1;
        }

        delete[] field1;
        delete[] field2;
        delete[] field3;
        delete[] field4;
        delete[] field5;
        delete[] field6;
        return 0;
    }

    size_t getSize(std::fstream &file) {
        size_t beg, end, size;
        if (file) {
            beg = file.tellg();
            file.seekg(0, std::ios::end);
            end = file.tellg();
            size = end - beg;
            file.seekg(0, std::ios::beg);
            return size;
        }
        return 0;
    }
}
