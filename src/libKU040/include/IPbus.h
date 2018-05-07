#ifndef IPBUS_H__
#define IPBUS_H__

#include <exception>
#include <cstdint>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class IPbusException : public std::exception {
    public:
        IPbusException(const std::string &ErrorMessage)
        {
            Error = ErrorMessage;
        }

        ~IPbusException() throw() {};

        const char* what() const throw()
        {
            return Error.c_str();
        }

    private:
        std::string Error;
};

class IPbus {
	public:
		// constructor/destructor
		IPbus();
		IPbus(const std::string &host, const unsigned int port = 50001);
		~IPbus();

		// Connect
		void Connect(const std::string &host, const unsigned int port = 50001);
		void Disconnect();
		bool IsConnected();

		// read values
		uint32_t Read(uint32_t baseaddr);
		ssize_t Read(uint32_t baseaddr, uint32_t *buffer, size_t words, bool non_inc = false);

		// write values
		void Write(uint32_t baseaddr, uint32_t value);
		ssize_t Write(uint32_t baseaddr, uint32_t *buffer, size_t words, bool non_inc = false);

		// Read/Modify/Write
		void RMWbits(uint32_t baseaddr, uint32_t and_term, uint32_t or_term);
		void RMWsum(uint32_t baseaddr, uint32_t addend);


	private:
		// socket vars
		int m_socketfd;
		struct sockaddr_in m_server;

		// IPbus IDs
		unsigned int m_packet_id;
		unsigned int m_transaction_id;
};

#endif // IPBUS_H__
