#ifndef SIMPLEI2C_H__
#define SIMPLEI2C_H__

#include "SpecCom.h"
#include <cstdint>

class SimpleI2C {
public:
	SimpleI2C(SpecCom *arg_spec);
	~SimpleI2C();
	ssize_t read(uint8_t dev, uint8_t *buf, size_t len);
	uint8_t read(uint8_t dev);
	ssize_t write(uint8_t dev, uint8_t *buf, size_t len);
	void write(uint8_t dev, uint8_t value);
	bool detectSlave(uint8_t dev);

private:
	bool busyWait();
	SpecCom *m_spec;
};


#endif // SIMPLEI2C_H__
