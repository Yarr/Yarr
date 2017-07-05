/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-II
 * Description: a class to handle pixel models
 */

#include "PixelModel.h"

#include "Gauss.h"

using namespace Gauss;

PixelModel::PixelModel(float _Vthin_mean, float _Vthin_sigma, float _TDACVbp_mean, float _TDACVbp_sigma, float _noise_sigma_mean, float _noise_sigma_sigma)
{
	Vthin_mean = _Vthin_mean;
	Vthin_sigma = _Vthin_sigma;
	Vthin_gauss = rand_normal(Vthin_mean, Vthin_sigma, 0);

	TDACVbp_mean = _TDACVbp_mean;
	TDACVbp_sigma = _TDACVbp_sigma;
	TDACVbp_gauss = rand_normal(TDACVbp_mean, TDACVbp_sigma, 0);

	noise_sigma_mean = _noise_sigma_mean;
	noise_sigma_sigma = _noise_sigma_sigma;
	noise_sigma_gauss = rand_normal(noise_sigma_mean, noise_sigma_sigma, 0);

}
PixelModel::PixelModel(float _Vthin_mean, float _Vthin_sigma, float _Vthin_gauss, float _TDACVbp_mean, float _TDACVbp_sigma, float _TDACVbp_gauss, float _noise_sigma_mean, float _noise_sigma_sigma, float _noise_sigma_gauss)
{
	Vthin_mean = _Vthin_mean;
	Vthin_sigma = _Vthin_sigma;
	Vthin_gauss = _Vthin_gauss;

	TDACVbp_mean = _TDACVbp_mean;
	TDACVbp_sigma = _TDACVbp_sigma;
	TDACVbp_gauss = _TDACVbp_gauss;

	noise_sigma_mean = _noise_sigma_mean;
	noise_sigma_sigma = _noise_sigma_sigma;
	noise_sigma_gauss = _noise_sigma_gauss;
}
PixelModel::~PixelModel()
{
}

// functions for modeling pixel responses
float PixelModel::calculateThreshold(uint32_t Vthin_Fine, uint32_t Vthin_Coarse, uint32_t TDAC)
{
	float modelVthin = Vthin_gauss * Vthin_Fine + Vthin_gauss * Vthin_Coarse * 128;
	float modelTDAC = 30.0 * TDAC;
	float threshold = modelVthin - modelTDAC;

	if (threshold < 0)
	{
		threshold = 0;
	}

	return threshold;
}

float PixelModel::calculateNoise()
{
	return rand_normal(0, noise_sigma_gauss, 1);
}

uint32_t PixelModel::calculateToT(float charge)
{
	return (uint32_t) (charge * 9.0 / 16000.0) + 1;
}
