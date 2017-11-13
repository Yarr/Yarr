/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-XI
 * Description: a class to handle rd53a diff pixel models
 */

#ifndef RD53ADiffPIXELMODEL
#define RD53ADiffPIXELMODEL

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

class Rd53aDiffPixelModel {
	public:
		Rd53aDiffPixelModel(float _VthDiff_mean, float _VthDiff_sigma, float _noise_sigma_mean, float _noise_sigma_sigma);
		Rd53aDiffPixelModel(float _VthDiff_mean, float _VthDiff_sigma, float _VthDiff_gauss, float _noise_sigma_mean, float _noise_sigma_sigma, float _noise_sigma_gauss);
		~Rd53aDiffPixelModel();

                // Vth is used to apply the same Gaussian smearing to both Vth1 and Vth2
		float VthDiff_mean;
		float VthDiff_sigma;
		float VthDiff_gauss;
		float noise_sigma_mean;
		float noise_sigma_sigma;
		float noise_sigma_gauss;

		// functions for modeDiffg pixel responses
		float calculateThreshold(uint32_t Vth1Diff, uint32_t Vth2Diff);
		float calculateNoise();
		uint32_t calculateToT(float charge);

	private:
};

#endif
