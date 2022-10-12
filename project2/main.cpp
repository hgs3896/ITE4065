#include "ThreadPool.hpp"
#include "WaitFreeAtomicSnapshot.hpp"
#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <iomanip>
#include <string>
#include <random>

int main(int argc, char const *argv[])
{
    if(argc != 2){
        std::cout << "Usage: ./main [number of threads]" << std::endl;
        return 0;
    }

    std::vector<std::future<size_t>> jobs;

    int n = std::stoi(argv[1]);
    ThreadPool pool(n);
    WaitFreeAtomicSnapshot<uint64_t> snapshot(n);
    jobs.reserve(n);

    for(int i = 0; i < n; i++){
        jobs.push_back(pool.EnqueueJob([tid = i, &snapshot](){
            std::mt19937_64 random_generator(tid);

            auto start = std::chrono::high_resolution_clock::now();
            while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(60)){
                snapshot.update(tid, random_generator());
                auto clean_collect = snapshot.scan();
            }
            return snapshot.getNumberOfUpdates(tid);
        }));
    }

    size_t tid = 0;
    size_t total_updates = 0;
    for(auto&& job : jobs){
        auto num_updates = job.get();
        total_updates += num_updates;
        std::cout << "Thread[" << std::right << std::setw(2) << tid++ << "] " << num_updates << std::endl;
    }

    std::cout << "Total Updates made: " << total_updates << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
