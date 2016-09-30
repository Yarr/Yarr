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
} Fei4Hit;

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE = "YarrFei4";

  // Declare a new class that inherits from DataConverterPlugin
  class ExampleConverterPlugin : public DataConverterPlugin {

  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event &bore, const Configuration &cnf) {
      //m_exampleparam = bore.GetTag("EXAMPLE", 0);
#ifndef WIN32 // some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
      // If the event type is used for different sensors
      // they can be differentiated here
      std::string sensortype = "Fe65p2";
      // Create a StandardPlane representing one sensor plane
      int id = 0;
      StandardPlane plane(id, EVENT_TYPE, sensortype);
      // Set the number of pixels
      int width = 64, height = 64;
      plane.SetSizeRaw(width, height);

      const RawDataEvent & my_ev = dynamic_cast<const RawDataEvent &>(ev);
      eudaq::RawDataEvent::data_t block0=my_ev.GetBlock(0);

      uint32_t mytag = *((uint32_t*) (&block0[0]));
      plane.SetTLUEvent(mytag);

      eudaq::RawDataEvent::data_t block1=my_ev.GetBlock(1);
      uint16_t myl1id = *((uint16_t*) (&block1[0]));
      
      eudaq::RawDataEvent::data_t block2=my_ev.GetBlock(2);
      uint16_t mybcid = *((uint16_t*) (&block2[0]));

      eudaq::RawDataEvent::data_t block3=my_ev.GetBlock(3);
      uint16_t mnHits = *((uint16_t*) (&block3[0]));

      eudaq::RawDataEvent::data_t block4=my_ev.GetBlock(4);
      for (unsigned i=0; i<mnHits; i+=sizeof(Fei4Hit)) {
	  Fei4Hit yhit = *((Fei4Hit *) &block4[i]);
	  int tot=yhit.tot;
	  int col=yhit.col;
	  int row=yhit.row;
	  plane.PushPixel(col,row,tot,false,mybcid);
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
