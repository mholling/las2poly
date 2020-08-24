#ifndef ESTIMATOR_HPP
#define ESTIMATOR_HPP

#include <cstddef>
#include <vector>
#include <algorithm>
#include <random>
#include <iterator>
#include <cmath>
#include <utility>

template <typename Model, typename Datum, std::size_t sample_size>
class Estimator {
	double noise_threshold;
	unsigned int min_consensus;
	unsigned int iterations;

public:
	Estimator(double noise_threshold, unsigned int min_consensus, unsigned int iterations) : noise_threshold(noise_threshold), min_consensus(min_consensus), iterations(iterations) { }

	template <typename Data>
	bool operator()(const Data &data, Model &model) const {
		if (data.size() < min_consensus)
			return false;

		std::vector<std::pair<Model, double>> results;
		for (unsigned int iteration = 0; iteration < iterations; ++iteration) {
			std::vector<Datum> sample, consensus;
			std::sample(data.begin(), data.end(), std::back_inserter(sample), sample_size, std::mt19937(std::random_device()()));

			Model hypothesis(sample);
			copy_if(data.begin(), data.end(), std::back_inserter(consensus), [&](const auto &datum) {
				return std::abs(hypothesis.error(datum)) < noise_threshold;
			});

			if (consensus.size() < min_consensus)
				continue;

			Model candidate(consensus);
			const auto loss = std::accumulate(data.begin(), data.end(), 0.0, [&](const auto &sum, const auto &datum) {
				auto error = std::abs(candidate.error(datum));
				return sum + (error > noise_threshold ? noise_threshold * noise_threshold : error * error);
			});
			results.push_back(std::make_pair(candidate, loss));
		}

		if (results.empty())
			return false;

		model = min_element(results.begin(), results.end(), [](const auto &result1, const auto &result2) {
			return result1.second < result2.second;
		})->first;
		return true;
	}
};

#endif
