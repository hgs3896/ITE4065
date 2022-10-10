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
                    EXPECT_LE(collect->items[0]->data, collect->items[1]->data);
                    // for(auto i = 0; i < N; ++i){
                        // if(i == tid)
                        //     EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        // else
                        //     EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    // }

                    // if(tid==1){
                    //     std::cout << "Old: " << old_collect << std::endl;
                    //     std::cout << "New: " << collect << std::endl;
                    // }

                    old_collect = std::move(collect);
                }
            }));
        }

        for(auto&& job : jobs){
            job.wait();
        }

        for(size_t tid = 0; tid < N; ++tid){
            std::cout << "ScanTest02: " << snapshot.getNumberOfUpdates(tid) << std::endl;
        }
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
                    for(auto i = 0; i < N; ++i){
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

        for(size_t tid = 0; tid < N; ++tid){
            std::cout << "ScanTest04: " << snapshot.getNumberOfUpdates(tid) << std::endl;
        }
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
                    for(auto i = 0; i < N; ++i){
                        if(i == tid)
                            EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        else
                            EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    }
                    
                    // if(tid==0){
                    //     std::cout << "Old: " << old_collect << std::endl;
                    //     std::cout << "New: " << collect << std::endl;
                    // }

                    old_collect = std::move(collect);
                    ++v;
                }
            }));
        }

        for(auto&& job : jobs){
            job.wait();
        }

        for(size_t tid = 0; tid < N; ++tid){
            std::cout << "ScanTest08: " << snapshot.getNumberOfUpdates(tid) << std::endl;
        }
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
                    for(auto i = 0; i < N; ++i){
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

        for(size_t tid = 0; tid < N; ++tid){
            std::cout << "ScanTest16: " << snapshot.getNumberOfUpdates(tid) << std::endl;
        }
    }

    TEST_F(WaitFreeAtomicSnapshotTest, ScanTest32) {
        constexpr auto N = 32;

        WaitFreeAtomicSnapshot<uint32_t> snapshot(N);
        std::vector<std::future<void>> jobs;
        // std::vector<std::future<std::vector<std::array<uint32_t, N>>>> jobs;
        jobs.reserve(N);

        for(size_t tid = 0; tid < N; ++tid){
            jobs.push_back(pool.EnqueueJob([&snapshot, tid](){
                // std::vector<std::array<uint32_t, N>> results;
                // results.reserve(500000);

                auto start = std::chrono::high_resolution_clock::now();
                size_t v = 0;
                
                auto old_collect = snapshot.scan();
                
                while(std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(secs)){
                    snapshot.update(tid, v);
                    auto collect = snapshot.scan();
                    
                    // auto& result = results.emplace_back();
                    for(auto i = 0; i < N; ++i){
                        // result[i] = collect->items[i]->data;
                        if(i == tid)
                            EXPECT_TRUE(old_collect->items[i]->version + 1 == collect->items[i]->version);
                        else
                            EXPECT_TRUE(old_collect->items[i]->version <= collect->items[i]->version);
                    }

                    old_collect = std::move(collect);

                    ++v;
                }

                // return results;
            }));
        }

        // std::vector<std::array<uint32_t, N>> all;
        for(auto&& job : jobs){
            // auto result = job.get();
            // all.insert(all.end(), result.begin(), result.end());
            job.wait();
        }
        // std::sort(all.begin(), all.end());
        // for(auto& x : all){
        //     for(auto& y : x){
        //         std::cout << y << " ";
        //     }
        //     std::cout << std::endl;
        // }
        
        for(size_t tid = 0; tid < N; ++tid){
            std::cout << "ScanTest32: " << snapshot.getNumberOfUpdates(tid) << std::endl;
        }
    }
}