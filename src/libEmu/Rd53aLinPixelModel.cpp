/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-X
 * Description: a class to handle rd53a lin pixel models
 */

#include "Rd53aLinPixelModel.h"

#include "Gauss.h"

using namespace Gauss;

Rd53aLinPixelModel::Rd53aLinPixelModel(float _VthresholdLin_mean, float _VthresholdLin_sigma, float _noise_sigma_mean, float _noise_sigma_sigma)
{
	VthresholdLin_mean = _VthresholdLin_mean;
	VthresholdLin_sigma = _VthresholdLin_sigma;
	VthresholdLin_gauss = rand_normal(VthresholdLin_mean, VthresholdLin_sigma, 0);

	noise_sigma_mean = _noise_sigma_mean;
	noise_sigma_sigma = _noise_sigma_sigma;
	noise_sigma_gauss = rand_normal(noise_sigma_mean, noise_sigma_sigma, 0);

}
Rd53aLinPixelModel::Rd53aLinPixelModel(float _VthresholdLin_mean, float _VthresholdLin_sigma, float _VthresholdLin_gauss, float _noise_sigma_mean, float _noise_sigma_sigma, float _noise_sigma_gauss)
{
	VthresholdLin_mean = _VthresholdLin_mean;
	VthresholdLin_sigma = _VthresholdLin_sigma;
	VthresholdLin_gauss = _VthresholdLin_gauss;

	noise_sigma_mean = _noise_sigma_mean;
	noise_sigma_sigma = _noise_sigma_sigma;
	noise_sigma_gauss = _noise_sigma_gauss;
}
Rd53aLinPixelModel::~Rd53aLinPixelModel()= default;

// functions for modeling pixel responses
float Rd53aLinPixelModel::calculateThreshold(uint32_t VthresholdLin, int TDAC)
{
  float modelVth = VthresholdLin_gauss/100. * VthresholdLin;
  float modelTDAC = (TDAC - 8) * 3;
  float threshold = modelVth - modelTDAC;

  if (threshold < 0) threshold = 0;

  return threshold;
}

float Rd53aLinPixelModel::calculateNoise()
{
	return rand_normal(0, noise_sigma_gauss, 1);
}

uint32_t Rd53aLinPixelModel::calculateToT(float charge)
{
  return uint32_t(1.51625+7.29853*charge/10000.);
}
