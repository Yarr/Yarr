#include "Gauss.h"

#include <fstream>

#include "storage.hpp"
using namespace Gauss;

void makeFE(float Vthreshold_mean, float Vthreshold_sigma, float noise_sigma_mean, float noise_sigma_sigma, std::vector<float> &Vthreshold_mean_vector, std::vector<float> &Vthreshold_sigma_vector, std::vector<float> &Vthreshold_gauss_vector, std::vector<float> &noise_sigma_mean_vector, std::vector<float> &noise_sigma_sigma_vector, std::vector<float> &noise_sigma_gauss_vector, bool isSync=false);

int main(int argc, char * argv[])
{
	if (argc == 1)
	{
		fprintf(stderr, "ERROR - too few arguments provided\n");
		printf("usage: %s filename.json\n", argv[0]);
		return 1;
	}

	std::string output_file = argv[1];

	if (output_file.size() == 0)
	{
		fprintf(stderr, "ERROR - output_file not provided\n");
		return 1;
	}

	std::ofstream file(output_file);
	nlohmann::json j;

        std::vector<float> Vthreshold_mean_vector; Vthreshold_mean_vector.reserve(400 * 192);
        std::vector<float> Vthreshold_sigma_vector; Vthreshold_sigma_vector.reserve(400 * 192);
        std::vector<float> Vthreshold_gauss_vector; Vthreshold_gauss_vector.reserve(400 * 192);
        std::vector<float> noise_sigma_mean_vector; noise_sigma_mean_vector.reserve(400 * 192);
        std::vector<float> noise_sigma_sigma_vector; noise_sigma_sigma_vector.reserve(400 * 192);
        std::vector<float> noise_sigma_gauss_vector; noise_sigma_gauss_vector.reserve(400 * 192);

	makeFE(0, 10, 80, 10, Vthreshold_mean_vector, Vthreshold_sigma_vector, Vthreshold_gauss_vector, noise_sigma_mean_vector, noise_sigma_sigma_vector, noise_sigma_gauss_vector, true);		// Sync
	makeFE(0, 10, 80, 10, Vthreshold_mean_vector, Vthreshold_sigma_vector, Vthreshold_gauss_vector, noise_sigma_mean_vector, noise_sigma_sigma_vector, noise_sigma_gauss_vector);		// Lin
	makeFE(0, 60, 40, 10, Vthreshold_mean_vector, Vthreshold_sigma_vector, Vthreshold_gauss_vector, noise_sigma_mean_vector, noise_sigma_sigma_vector, noise_sigma_gauss_vector);		// Diff
	
	j["Vthreshold_mean_vector"] = Vthreshold_mean_vector;
	j["Vthreshold_sigma_vector"] = Vthreshold_sigma_vector;
	j["Vthreshold_gauss_vector"] = Vthreshold_gauss_vector;
	j["noise_sigma_mean_vector"] = noise_sigma_mean_vector;
	j["noise_sigma_sigma_vector"] = noise_sigma_sigma_vector;
	j["noise_sigma_gauss_vector"] = noise_sigma_gauss_vector;
	file << j;
	file.close();
}

void makeFE(float Vthreshold_mean, float Vthreshold_sigma, float noise_sigma_mean, float noise_sigma_sigma, std::vector<float> &Vthreshold_mean_vector, std::vector<float> &Vthreshold_sigma_vector, std::vector<float> &Vthreshold_gauss_vector, std::vector<float> &noise_sigma_mean_vector, std::vector<float> &noise_sigma_sigma_vector, std::vector<float> &noise_sigma_gauss_vector, bool isSync){
  unsigned nCol = isSync ? 128 : 136;
  unsigned nRow = 192;
  for (unsigned col = 1; col <= nCol; col++){
    for (unsigned row = 1; row <= nRow; row++){
      float Vthreshold_gauss = rand_normal(Vthreshold_mean, Vthreshold_sigma, 1);
      float noise_sigma_gauss = rand_normal(noise_sigma_mean, noise_sigma_sigma, 0);
      Vthreshold_mean_vector.push_back(Vthreshold_mean);
      Vthreshold_sigma_vector.push_back(Vthreshold_sigma);
      Vthreshold_gauss_vector.push_back(Vthreshold_gauss);
      noise_sigma_mean_vector.push_back(noise_sigma_mean);
      noise_sigma_sigma_vector.push_back(noise_sigma_sigma);
      noise_sigma_gauss_vector.push_back(noise_sigma_gauss);
    }
  }
}
