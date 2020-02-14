#ifndef STRIPMODEL
#define STRIPMODEL

#include <cstdint>

class StripModel {
  public:
    StripModel();
    ~StripModel();

    void setValue(float, float, float, float);
    
    float calculateThreshold(uint8_t BVT, uint8_t TrimDAC, uint8_t TrimRange);
    float calculateBVT(uint8_t BVT);
    float calculateTrimDAC(uint8_t TrimDAC, uint8_t TrimRange);
    
    float calculateNoise();
    static float calculateInjection(uint16_t BCAL);

    bool calculateHit(uint16_t BCAL, uint8_t BVT, uint8_t TrimDAC, uint8_t TrimRange);

    static float gain_function(float charge);
    
  protected:
    float vthreshold_mean;
    float vthreshold_sigma;
    float noise_occupancy_mean;
    float noise_occupancy_sigma;

    static constexpr float Injection_Capacitor = 60.; // fF
};

#endif
