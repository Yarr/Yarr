#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

typedef struct {
  unsigned col : 7;
  unsigned row : 9;
  unsigned tot : 5;
  unsigned unused : 11;
} YHit;

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE = "Example";

  // Declare a new class that inherits from DataConverterPlugin
  class ExampleConverterPlugin : public DataConverterPlugin {

  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event &bore, const Configuration &cnf) {
      m_exampleparam = bore.GetTag("EXAMPLE", 0);
#ifndef WIN32 // some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
    }

    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event &ev) const {
      static const unsigned TRIGGER_OFFSET = 8;
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev)) {
        // This is just an example, modified it to suit your raw data format
        // Make sure we have at least one block of data, and it is large enough
        if (rev->NumBlocks() > 0 &&
            rev->GetBlock(0).size() >= (TRIGGER_OFFSET + sizeof(short))) {
          // Read a little-endian unsigned short from offset TRIGGER_OFFSET
          return getlittleendian<unsigned short>(
						 &rev->GetBlock(0)[TRIGGER_OFFSET]);
        }
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
      // If the event type is used for different sensors
      // they can be differentiated here
      std::string sensortype = "example";
      // Create a StandardPlane representing one sensor plane
      int id = 0;
      StandardPlane plane(id, EVENT_TYPE, sensortype);
      // Set the number of pixels
      int width = 100, height = 50;
      plane.SetSizeRaw(width, height);
      // Set the trigger ID
      plane.SetTLUEvent(GetTriggerID(ev));
      // Add the plane to the StandardEvent

      //This is the new bit for YARR.  I've tested it with YARR standalone and it workd, but I'm not totally sure what Timon is going to put in these blocks (such as if there will be a header or so)!  Therefore, this will need some minor modifications once the Producer is done.

      const RawDataEvent & my_ev = dynamic_cast<const RawDataEvent &>(ev);
      eudaq::RawDataEvent::data_t block0=my_ev.GetBlock(0);
      
      unsigned it = 0;
      //Only setup for data at the moment ! (i.e. not headers)
      uint16_t myl1id = (uint16_t) *(&block0[it]); it += sizeof(uint16_t);
      uint16_t mybcid = (uint16_t) *(&block0[it]); it += sizeof(uint16_t);
      uint16_t mnHits = (uint16_t) *(&block0[it]); it += sizeof(uint16_t);
      
      if(myl1id==0 && mybcid==0){
	std::cout<<"Event not valid. Not filling Planes."<<std::endl;
	return false;
      }
      
      while (it < block0.size()){
	for (unsigned i=0; i<mnHits; i++) {
	  YHit yhit = *(YHit *) &block0[it]; it += sizeof(YHit);
	  int tot=yhit.tot;
	  int col=yhit.col;
	  int row=yhit.row;
	  plane.PushPixel(col,row,tot,false,0);
	}
      }
      
      sev.AddPlane(plane);
      // Indicate that data was successfully converted
      return true;
    }

#if USE_LCIO
    // This is where the conversion to LCIO is done
    virtual lcio::LCEvent *GetLCIOEvent(const Event * /*ev*/) const {
      return 0;
    }
#endif

  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    ExampleConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0) {}

    // Information extracted in Initialize() can be stored here:
    unsigned m_exampleparam;

    // The single instance of this converter plugin
    static ExampleConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  ExampleConverterPlugin ExampleConverterPlugin::m_instance;

} // namespace eudaq
