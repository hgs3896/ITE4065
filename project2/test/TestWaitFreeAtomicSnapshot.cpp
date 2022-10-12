#include <array>
#include <algorithm>
#include "gtest/gtest.h"
#include "ThreadPool.hpp"
#include "WaitFreeAtomicSnapshot.hpp"

namespace {
    class WaitFreeAtomicSnapshotTest : public testing::Test {
    protected:
        static constexpr size_t secs = 10;
        ThreadPool pool;
        WaitFreeAtomicSnapshotTest(): pool(32) {}
    };

    TEST_F(WaitFreeAtomicSnapshotTest, ScanTest01) {
        WaitFreeAtomicSnapshot<uint32_t> snapshot(1);
        
        size_t v = 0;
        auto start = std::chrono::high_resolution_clock::now();

        while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(secs)){
            snapshot.update(0, v);
            auto collect = snapshot.scan();
            ASSERT_EQ(collect->items[0]->data, v);
            ++v;
        }

        std::cout << "ScanTest01: " << snapshot.getNumberOfUpdates(0) << std::endl;
    }

    TEST_F(WaitFreeAtomicSnapshotTest, ScanTest02) {
        constexpr auto N = 2;

        WaitFreeAtomicSnapshot<uint32_t> snapshot(N);
        std::vector<std::future<void>> jobs;
        jobs.reserve(N);

        for(size_t tid = 0; tid < N; ++tid){
            jobs.push_back(pool.EnqueueJob([&snapshot, tid](){
                auto start = std::chrono::high_resolution_clock::now();
                
                auto old_collect = snapshot.scan();
                while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(secs)){
                    if(tid == 0)
                        snapshot.update(tid, old_collect->items[0]->data + 1);
                    else
                        snapshot.update(tid, old_collect->items[0]->data + 1);
                    auto collect = snapshot.scan();
                    
                    for(size_t i = 0; i < N; ++i){
                        if(i == tid)
                            EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        else
                            EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    }

                    old_collect = std::move(collect);
                }
            }));
        }

        for(auto&& job : jobs){
            job.wait();
        }

        size_t total_updates = 0;
        for(size_t tid = 0; tid < N; ++tid){
            auto num_updates = snapshot.getNumberOfUpdates(tid);
            total_updates += num_updates;
            std::cout << "ScanTest02[" << std::right << std::setw(2) << tid << "]: " << num_updates << std::endl;
        }
        std::cout << "ScanTest02" << ": " << total_updates << std::endl;
    }

    TEST_F(WaitFreeAtomicSnapshotTest, ScanTest04) {
        constexpr auto N = 4;

        WaitFreeAtomicSnapshot<uint32_t> snapshot(N);
        std::vector<std::future<void>> jobs;
        jobs.reserve(N);

        for(size_t tid = 0; tid < N; ++tid){
            jobs.push_back(pool.EnqueueJob([&snapshot, tid](){
                auto start = std::chrono::high_resolution_clock::now();
                size_t v = 0;

                auto old_collect = snapshot.scan();
                while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(secs)){
                    snapshot.update(tid, v);
                    auto collect = snapshot.scan();
                    for(size_t i = 0; i < N; ++i){
                        if(i == tid)
                            EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        else
                            EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    }
                    old_collect = std::move(collect);

                    ++v;
                }
            }));
        }

        for(auto&& job : jobs){
            job.wait();
        }

        size_t total_updates = 0;
        for(size_t tid = 0; tid < N; ++tid){
            auto num_updates = snapshot.getNumberOfUpdates(tid);
            total_updates += num_updates;
            std::cout << "ScanTest04[" << std::right << std::setw(2) << tid << "]: " << num_updates << std::endl;
        }
        std::cout << "ScanTest04" << ": " << total_updates << std::endl;
    }

    TEST_F(WaitFreeAtomicSnapshotTest, ScanTest08) {
        constexpr auto N = 8;

        WaitFreeAtomicSnapshot<uint32_t> snapshot(N);
        std::vector<std::future<void>> jobs;
        jobs.reserve(N);

        for(size_t tid = 0; tid < N; ++tid){
            jobs.push_back(pool.EnqueueJob([&snapshot, tid](){
                auto start = std::chrono::high_resolution_clock::now();
                size_t v = 0;

                auto old_collect = snapshot.scan();
                while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(secs)){
                    snapshot.update(tid, v);
                    auto collect = snapshot.scan();
                    for(size_t i = 0; i < N; ++i){
                        if(i == tid)
                            EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        else
                            EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    }

                    old_collect = std::move(collect);
                    ++v;
                }
            }));
        }

        for(auto&& job : jobs){
            job.wait();
        }

        size_t total_updates = 0;
        for(size_t tid = 0; tid < N; ++tid){
            auto num_updates = snapshot.getNumberOfUpdates(tid);
            total_updates += num_updates;
            std::cout << "ScanTest08[" << std::right << std::setw(2) << tid << "]: " << num_updates << std::endl;
        }
        std::cout << "ScanTest08" << ": " << total_updates << std::endl;
    }

    TEST_F(WaitFreeAtomicSnapshotTest, ScanTest16) {
        constexpr auto N = 16;

        WaitFreeAtomicSnapshot<uint32_t> snapshot(N);
        std::vector<std::future<void>> jobs;
        jobs.reserve(N);

        for(size_t tid = 0; tid < N; ++tid){
            jobs.push_back(pool.EnqueueJob([&snapshot, tid](){
                auto start = std::chrono::high_resolution_clock::now();
                size_t v = 0;

                auto old_collect = snapshot.scan();
                while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(secs)){
                    snapshot.update(tid, v);
                    auto collect = snapshot.scan();
                    for(size_t i = 0; i < N; ++i){
                        if(i == tid)
                            EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        else
                            EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    }
                    old_collect = std::move(collect);

                    ++v;
                }
            }));
        }

        for(auto&& job : jobs){
            job.wait();
        }

        size_t total_updates = 0;
        for(size_t tid = 0; tid < N; ++tid){
            auto num_updates = snapshot.getNumberOfUpdates(tid);
            total_updates += num_updates;
            std::cout << "ScanTest16[" << std::right << std::setw(2) << tid << "]: " << num_updates << std::endl;
        }
        std::cout << "ScanTest16" << ": " << total_updates << std::endl;
    }

    TEST_F(WaitFreeAtomicSnapshotTest, ScanTest32) {
        constexpr auto N = 32;

        WaitFreeAtomicSnapshot<uint32_t> snapshot(N);
        std::vector<std::future<void>> jobs;
        jobs.reserve(N);

        for(size_t tid = 0; tid < N; ++tid){
            jobs.push_back(pool.EnqueueJob([&snapshot, tid](){

                auto start = std::chrono::high_resolution_clock::now();
                size_t v = 0;
                
                auto old_collect = snapshot.scan();
                
                while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(secs)){
                    snapshot.update(tid, v);
                    auto collect = snapshot.scan();
                    
                    for(size_t i = 0; i < N; ++i){
                        if(i == tid)
                            EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        else
                            EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    }

                    old_collect = std::move(collect);

                    ++v;
                }
            }));
        }

        for(auto&& job : jobs){
            job.wait();
        }
        
        size_t total_updates = 0;
        for(size_t tid = 0; tid < N; ++tid){
            auto num_updates = snapshot.getNumberOfUpdates(tid);
            total_updates += num_updates;
            std::cout << "ScanTest32[" << std::right << std::setw(2) << tid << "]: " << num_updates << std::endl;
        }
        std::cout << "ScanTest32" << ": " << total_updates << std::endl;
    }
}