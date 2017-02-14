#ifndef RCECOM_H
#define RCECOM_H
#include "PgpModL.hh"
#include "RCDImasterL.hh"
#include "Receiver.hh"
#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>
class RceCom : public PgpTrans::Receiver {
    public:
        virtual uint32_t getCurSize() ;
        virtual bool isEmpty();
        virtual uint32_t read32();
        virtual uint32_t readBlock32(uint32_t *buf, uint32_t length);
        virtual void write32(uint32_t);
	virtual void releaseFifo();
    protected:
        RceCom();
        ~RceCom();
    private:
	void receive(PgpTrans::PgpData *pgpdata);
	PgpTrans::RCDImaster* pgp;
	std::vector<uint32_t> txfifo;
	std::queue<uint32_t> rxfifo;
	uint32_t m_counter;
	std::mutex m_lock;

};

#endif
