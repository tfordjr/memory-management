// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#ifndef RNG_H
#define RNG_H

#include <random>
#include <chrono>

int generate_random_number(int min, int max, int pid) {  // pseudo rng for random child workload
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count() * pid;
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);
    int random_number = distribution(generator);
    return random_number;
}

#endif