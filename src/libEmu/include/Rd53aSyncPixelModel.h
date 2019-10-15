/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-X
 * Description: a class to handle rd53a sync pixel models
 */

#ifndef RD53ASYNCPIXELMODEL
#define RD53ASYNCPIXELMODEL

#include <cstdint>

class Rd53aSyncPixelModel {
	public:
		Rd53aSyncPixelModel(float _VthresholdSync_mean, float _VthresholdSync_sigma, float _noise_sigma_mean, float _noise_sigma_sigma);
		Rd53aSyncPixelModel(float _VthresholdSync_mean, float _VthresholdSync_sigma, float _VthresholdSync_gauss, float _noise_sigma_mean, float _noise_sigma_sigma, float _noise_sigma_gauss);
		~Rd53aSyncPixelModel();

		float VthresholdSync_mean;
		float VthresholdSync_sigma;
		float VthresholdSync_gauss;
		float noise_sigma_mean;
		float noise_sigma_sigma;
		float noise_sigma_gauss;

		// functions for modeling pixel responses
		float calculateThreshold(uint32_t VthresholdSync);
		float calculateNoise();
		uint32_t calculateToT(float charge);

	private:
};

#endif
