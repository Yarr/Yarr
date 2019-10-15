/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-X
 * Description: a class to handle rd53a lin pixel models
 */

#include "Rd53aSyncPixelModel.h"

#include "Gauss.h"

using namespace Gauss;

Rd53aSyncPixelModel::Rd53aSyncPixelModel(float _VthresholdSync_mean, float _VthresholdSync_sigma, float _noise_sigma_mean, float _noise_sigma_sigma)
{
	VthresholdSync_mean = _VthresholdSync_mean;
	VthresholdSync_sigma = _VthresholdSync_sigma;
	VthresholdSync_gauss = rand_normal(VthresholdSync_mean, VthresholdSync_sigma, 0);

	noise_sigma_mean = _noise_sigma_mean;
	noise_sigma_sigma = _noise_sigma_sigma;
	noise_sigma_gauss = rand_normal(noise_sigma_mean, noise_sigma_sigma, 0);

}
Rd53aSyncPixelModel::Rd53aSyncPixelModel(float _VthresholdSync_mean, float _VthresholdSync_sigma, float _VthresholdSync_gauss, float _noise_sigma_mean, float _noise_sigma_sigma, float _noise_sigma_gauss)
{
	VthresholdSync_mean = _VthresholdSync_mean;
	VthresholdSync_sigma = _VthresholdSync_sigma;
	VthresholdSync_gauss = _VthresholdSync_gauss;

	noise_sigma_mean = _noise_sigma_mean;
	noise_sigma_sigma = _noise_sigma_sigma;
	noise_sigma_gauss = _noise_sigma_gauss;
}
Rd53aSyncPixelModel::~Rd53aSyncPixelModel()
{
}

// functions for modeling pixel responses
float Rd53aSyncPixelModel::calculateThreshold(uint32_t VthresholdSync)
{
	float threshold = VthresholdSync + VthresholdSync_gauss;

	if (threshold < 0) threshold = 0;

	return threshold;
}

float Rd53aSyncPixelModel::calculateNoise()
{
	return rand_normal(0, noise_sigma_gauss, 1);
}

uint32_t Rd53aSyncPixelModel::calculateToT(float charge)
{
	return 1;
}
