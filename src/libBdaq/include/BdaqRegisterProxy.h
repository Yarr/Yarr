#ifndef BDAQREGISTERPROXY_H
#define BDAQREGISTERPROXY_H

// Register Layer Helper Class.

#include <functional>
#include <cstdint>

class BdaqRegisterProxy {
	public:
		BdaqRegisterProxy(const std::function<uint64_t(void)> &_read, const std::function<void(uint64_t)> &_write) : read(_read), write(_write) {}

		uint64_t operator=(uint64_t val);
		operator uint64_t () const;

	protected:
		std::function<uint64_t(void)> read;
		std::function<void(uint64_t)> write;
};

#endif
