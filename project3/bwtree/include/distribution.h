#pragma once

#include <random>
#include "zipf.h"

namespace util {
    enum class DistributionType{
        Uniform,
        Zipf,
        Gaussian,
    };

    template<DistributionType distribution_type>
    class Distribution {};

    template<>
    class Distribution<DistributionType::Uniform> {
        public:
            Distribution(int min, int max):
                dist(min, max), gen() {}
            ~Distribution() = default;

            inline int operator()() {
                return dist(gen);
            }

            inline void SetSeed(int seed) {
                gen.seed(seed);
            }
        private:
            std::uniform_int_distribution<int> dist;
            std::default_random_engine gen;
    };

    template<>
    class Distribution<DistributionType::Zipf> {
        public:
            Distribution(int min, int max, float alpha):
                min(min), dist(alpha, max-min+1) {}
            ~Distribution() = default;

            inline int operator()() {
                return (min + dist.Generate() - 1);
            }

            inline void SetSeed(int seed) {
                dist.SetSeed(seed);
            }
        private:
            int id, min;
            util::Zipf dist;
    };
}