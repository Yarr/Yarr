#include <iostream>
#include "SimpleI2C.h"
#include "I2CRegs.h"

SimpleI2C::SimpleI2C(SpecCom *arg_spec)
{
	m_spec = arg_spec;

	// stop I2C core
    m_spec->writeSingle(I2C_ADDR | I2C_CTRL, 0x0);

    // Program clock prescaler, assume 200MHz sys clock
    // value = (sys_clk/((5 x scl_clk) -1 )
    // We want around 1 MHz SCL clock
    uint32_t prescale = 50;
    m_spec->writeSingle(I2C_ADDR | I2C_PRE_LOW, prescale & 0xFF);
    m_spec->writeSingle(I2C_ADDR | I2C_PRE_HIGH, 0x0);
    
    // Start up core
    m_spec->writeSingle(I2C_ADDR | I2C_CTRL, I2C_CTRL_I2C_EN);
}

SimpleI2C::~SimpleI2C()
{
	// stop I2C core
    m_spec->writeSingle(I2C_ADDR | I2C_CTRL, 0x0);
}

bool SimpleI2C::busyWait()
{
    unsigned timeout_cnt = 0;
    while(timeout_cnt++ < I2C_TIMEOUT)
    {
    	uint32_t stat = m_spec->readSingle(I2C_ADDR | I2C_STAT);
    	if((stat & I2C_STAT_TIP) == 0)
    		return false;
    }

    return true;
    
    /*while ((m_spec->readSingle(I2C_ADDR | I2C_STAT) & I2C_STAT_TIP) == I2C_STAT_TIP &&
            timeout_cnt < I2C_TIMEOUT)
        timeout_cnt++;
    if (timeout_cnt >= I2C_TIMEOUT)
        return true;

    return false;*/
}

uint8_t SimpleI2C::read(uint8_t dev)
{
	uint8_t ret;

	read(dev, &ret, 1);

	return ret;
}

ssize_t SimpleI2C::read(uint8_t dev, uint8_t *buf, size_t len)
{
	// setup device address to communicate to
	m_spec->writeSingle(I2C_ADDR | I2C_TX, (dev << 1) | 0x1);
	m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_START);
	if(busyWait())
	{
		std::cerr << __PRETTY_FUNCTION__ << " : timeout when addressing slave!" << std::endl;
		return -1;
	}

	// check for ack
	if(m_spec->readSingle(I2C_ADDR | I2C_STAT) & I2C_STAT_ACK)
	{
//		std::cout << __PRETTY_FUNCTION__ << " : no ack from slave!" << std::endl;
		return -1;
	}

	// read data from I2C interface
	for(size_t i = 0; i < len; i++)
	{
		if(i == (len-1))
		{
			// nak + stop
			m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STOP);
		}
		else
		{
			m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_RD);
		}

		// wait
		if(busyWait())
		{
			std::cerr << __PRETTY_FUNCTION__ << " : timeout when reading from the slave!" << std::endl;
			return -1;
		}

		// get the data
		buf[i] = m_spec->readSingle(I2C_ADDR | I2C_RX);
	}

	return len;
}

void SimpleI2C::write(uint8_t dev, uint8_t value)
{
	write(dev, &value, 1);
}

ssize_t SimpleI2C::write(uint8_t dev, uint8_t *buf, size_t len)
{
	// setup device address to communicate to
	m_spec->writeSingle(I2C_ADDR | I2C_TX, (dev << 1) | 0x0);
	m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_START);
	if(busyWait())
	{
		std::cerr << __PRETTY_FUNCTION__ << " : timeout when addressing slave!" << std::endl;
		return -1;
	}

        // check for ack
        if(m_spec->readSingle(I2C_ADDR | I2C_STAT) & I2C_STAT_ACK)
        {
//                std::cout << __PRETTY_FUNCTION__ << " :	no ack from slave!" << std::endl;
                return -1;
        }

	// write to the I2C device
	for(size_t i = 0; i < len; i++)
	{
		// prepare data
		m_spec->writeSingle(I2C_ADDR | I2C_TX, buf[i]);

		if(i == (len-1))
		{
			// stop
			m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_STOP);
		}
		else
		{
			m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR);
		}

		// wait
		if(busyWait())
		{
			std::cerr << __PRETTY_FUNCTION__ << " : timeout when writing to the slave!" << std::endl;
			return -1;
		}
	}

	return len;
}

bool SimpleI2C::detectSlave(uint8_t dev)
{
	bool ret = false;

	// setup device address to communicate to
	m_spec->writeSingle(I2C_ADDR | I2C_TX, (dev << 1) | 0x1);
	m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_WR | I2C_CMD_START);
	if(busyWait())
	{
		std::cerr << __PRETTY_FUNCTION__ << " : timeout when addressing slave!" << std::endl;
		return false;
	}

	// check for ack
	if((m_spec->readSingle(I2C_ADDR | I2C_STAT) & I2C_STAT_ACK) == 0)
	{
//		std::cout << __PRETTY_FUNCTION__ << " : no ack from slave!" << std::endl;
		ret = true;
	}

	// try to read one byte
	m_spec->writeSingle(I2C_ADDR | I2C_CMD, I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STOP);
	if(busyWait())
	{
		std::cerr << __PRETTY_FUNCTION__ << " : timeout when reading from the slave!" << std::endl;
		return false;
	}

	return ret;
}
