#include "StripModel.h"
#include "Gauss.h"

StripModel::StripModel()
    : vthreshold_mean(1)
    , vthreshold_sigma(0)
    , noise_occupancy_mean(0)
    , noise_occupancy_sigma(0)
{}

StripModel::~StripModel(){}

void StripModel::setValue(float vth_mean, float vth_sigma, float noise_mean,
                          float noise_sigma)
{
    vthreshold_mean = vth_mean;
    vthreshold_sigma = vth_sigma;
    noise_occupancy_mean = noise_mean;
    noise_occupancy_sigma = noise_sigma;
}

/// Threshold
inline float StripModel::calculateBVT(uint8_t BVT)
{
    // 8-bit DAC for global threshold
    // Range: 0 - -550 mV
    float vth = BVT / 255. * 550.; // -mv
    // Smear
    return vth * Gauss::rand_normal(vthreshold_mean, vthreshold_sigma, true) / vthreshold_mean;
}

inline float StripModel::calculateTrimDAC(uint8_t TrimDAC, uint8_t Range)
{
    // 5-bit TrimDAC Range: 50 - 230 mB
    float trimRange = 50. + Range * (230.-50.) / 32;
    // 5-bit TrimDAC
    float vtrim = trimRange * TrimDAC / 32.; // mV
    return vtrim;
}

float StripModel::calculateThreshold(uint8_t BVT, uint8_t TrimDAC,
                                     uint8_t TrimRange)
{
    float vth_smear = this->calculateBVT(BVT);
    float vtrim = this->calculateTrimDAC(TrimDAC, TrimRange);
    float threshold = vth_smear - vtrim;
    
    return threshold > 0 ? threshold : 0;
}

//
// Pre amp
//
/// Noise charge
float StripModel::calculateNoise()
{
    return Gauss::rand_normal(noise_occupancy_mean, noise_occupancy_sigma, true); // fC
}

/// Charge injection
float StripModel::calculateInjection(uint16_t BCAL)
{
    // 9-bit DAC: 0 - 170 mV
    float vcal = BCAL / 512. * 0.170; // V
    // Charge
    return vcal * Injection_Capacitor; // fC
}

/// Gain
float StripModel::gain_function(float charge)
{   // TODO
    // For now: linear gain 80 mV/fC
    return charge * 80; // mV
}

/// Determine if there is a hit
bool StripModel::calculateHit(uint16_t BCAL, uint8_t BVT, uint8_t TrimDAC,
                              uint8_t TrimRange)
{
    float injected_charge = calculateInjection(BCAL);
    float noise_charge = calculateNoise();

    // After amplifier
    float voltage_inj = gain_function(injected_charge + noise_charge);

    // Threshold
    float vthreshold = calculateThreshold(BVT, TrimDAC, TrimRange);

    // Compare
    return voltage_inj > vthreshold;
}
