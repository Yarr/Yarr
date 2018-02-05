#include "Gauss.h"
#include "json.hpp"
#include <fstream>

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;
using namespace Gauss;

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

        std::vector<float> VthresholdLin_mean_vector; VthresholdLin_mean_vector.reserve(136 * 192);
        std::vector<float> VthresholdLin_sigma_vector; VthresholdLin_sigma_vector.reserve(136 * 192);
        std::vector<float> VthresholdLin_gauss_vector; VthresholdLin_gauss_vector.reserve(136 * 192);
        std::vector<float> noise_sigma_mean_vector; noise_sigma_mean_vector.reserve(136 * 192);
        std::vector<float> noise_sigma_sigma_vector; noise_sigma_sigma_vector.reserve(136 * 192);
        std::vector<float> noise_sigma_gauss_vector; noise_sigma_gauss_vector.reserve(136 * 192);

	for (unsigned col = 1; col <= 136; col++)
	{
		for (unsigned row = 1; row <= 192; row++)
		{
			float VthresholdLin_mean = 10;
			float VthresholdLin_sigma = 2;
			float VthresholdLin_gauss = rand_normal(VthresholdLin_mean, VthresholdLin_sigma, 0);
			float noise_sigma_mean = 400;
			float noise_sigma_sigma = 100;
			float noise_sigma_gauss = rand_normal(noise_sigma_mean, noise_sigma_sigma, 0);

			VthresholdLin_mean_vector.push_back(VthresholdLin_mean);
			VthresholdLin_sigma_vector.push_back(VthresholdLin_sigma);
			VthresholdLin_gauss_vector.push_back(VthresholdLin_gauss);
			noise_sigma_mean_vector.push_back(noise_sigma_mean);
			noise_sigma_sigma_vector.push_back(noise_sigma_sigma);
			noise_sigma_gauss_vector.push_back(noise_sigma_gauss);
		}
	}

	j["VthresholdLin_mean_vector"] = VthresholdLin_mean_vector;
	j["VthresholdLin_sigma_vector"] = VthresholdLin_sigma_vector;
	j["VthresholdLin_gauss_vector"] = VthresholdLin_gauss_vector;
	j["noise_sigma_mean_vector"] = noise_sigma_mean_vector;
	j["noise_sigma_sigma_vector"] = noise_sigma_sigma_vector;
	j["noise_sigma_gauss_vector"] = noise_sigma_gauss_vector;
	file << j;
	file.close();
}

