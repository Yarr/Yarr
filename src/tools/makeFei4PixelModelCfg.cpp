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

	std::vector<float> Vthin_mean_vector; Vthin_mean_vector.reserve(80 * 336);
	std::vector<float> Vthin_sigma_vector; Vthin_sigma_vector.reserve(80 * 336);
	std::vector<float> Vthin_gauss_vector; Vthin_gauss_vector.reserve(80 * 336);
	std::vector<float> TDACVbp_mean_vector; TDACVbp_mean_vector.reserve(80 * 336);
	std::vector<float> TDACVbp_sigma_vector; TDACVbp_sigma_vector.reserve(80 * 336);
	std::vector<float> TDACVbp_gauss_vector; TDACVbp_gauss_vector.reserve(80 * 336);
	std::vector<float> noise_sigma_mean_vector; noise_sigma_mean_vector.reserve(80 * 336);
	std::vector<float> noise_sigma_sigma_vector; noise_sigma_sigma_vector.reserve(80 * 336);
	std::vector<float> noise_sigma_gauss_vector; noise_sigma_gauss_vector.reserve(80 * 336);

	for (unsigned col = 1; col <= 80; col++)
	{
		for (unsigned row = 1; row <= 336; row++)
		{
			float Vthin_mean = 22;
			float Vthin_sigma = 4;
			float Vthin_gauss = rand_normal(Vthin_mean, Vthin_sigma, 0);
			float TDACVbp_mean = 1;
			float TDACVbp_sigma = 0;
			float TDACVbp_gauss = rand_normal(TDACVbp_mean, TDACVbp_sigma, 0);
			float noise_sigma_mean = 150;
			float noise_sigma_sigma = 15;
			float noise_sigma_gauss = rand_normal(noise_sigma_mean, noise_sigma_sigma, 0);

			Vthin_mean_vector.push_back(Vthin_mean);
			Vthin_sigma_vector.push_back(Vthin_sigma);
			Vthin_gauss_vector.push_back(Vthin_gauss);
			TDACVbp_mean_vector.push_back(TDACVbp_mean);
			TDACVbp_sigma_vector.push_back(TDACVbp_sigma);
			TDACVbp_gauss_vector.push_back(TDACVbp_gauss);
			noise_sigma_mean_vector.push_back(noise_sigma_mean);
			noise_sigma_sigma_vector.push_back(noise_sigma_sigma);
			noise_sigma_gauss_vector.push_back(noise_sigma_gauss);
		}
	}

	j["Vthin_mean_vector"] = Vthin_mean_vector;
	j["Vthin_sigma_vector"] = Vthin_sigma_vector;
	j["Vthin_gauss_vector"] = Vthin_gauss_vector;
	j["TDACVbp_mean_vector"] = TDACVbp_mean_vector;
	j["TDACVbp_sigma_vector"] = TDACVbp_sigma_vector;
	j["TDACVbp_gauss_vector"] = TDACVbp_gauss_vector;
	j["noise_sigma_mean_vector"] = noise_sigma_mean_vector;
	j["noise_sigma_sigma_vector"] = noise_sigma_sigma_vector;
	j["noise_sigma_gauss_vector"] = noise_sigma_gauss_vector;
	file << j;
	file.close();
}

