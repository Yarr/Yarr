#ifndef PGPTRANS_RECEIVER_HH
#define PGPTRANS_RECEIVER_HH

//#include "namespace_aliases.hh"

namespace PgpTrans {

  struct PgpData{
    unsigned char *header;
    unsigned *payload;
    unsigned payloadSize;};

  class Receiver {
  public:
    Receiver()  {}
    virtual ~Receiver(){}
    virtual void receive(PgpData* pgpdata)=0;
  };

}

#endif
