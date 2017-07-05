#ifndef RCECOM_H
#define RCECOM_H
#include "PGPmaster.hh"
#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>
class RceCom : public Rce::Receiver {
    public:
         RceCom();
        ~RceCom();
        virtual uint32_t getCurSize() ;
        virtual bool isEmpty();
        virtual uint32_t read32();
        virtual uint32_t readBlock32(uint32_t *buf, uint32_t length);
        virtual void write32(uint32_t);
	virtual void releaseFifo();
	Rce::PGPmaster* pgp; //FIXME
	
    protected:
    private:
	void receive(Rce::PgpData *pgpdata);
	std::vector<uint32_t> txfifo;
	std::queue<uint32_t> rxfifo;
	uint32_t m_counter;
	std::mutex m_lock;

};

#endif
