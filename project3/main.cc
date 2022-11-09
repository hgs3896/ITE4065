#include <algorithm>
#include <random>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

#include "bwtree.h"
#include "bwtree_test_util.h"
#include "multithread_test_util.h"
#include "timer.h"
#include "worker_pool.h"
#include "distribution.h"

int main(int argc, char *argv[]) {
  double alpha = 1.0;
  if(argc == 2)
    alpha = std::stod(argv[1]);

#if 0
  // util::Distribution<util::DistributionType::Uniform> dist(0, 10);
  util::Distribution<util::DistributionType::Zipf> dist(0, 10, alpha);
  
  std::vector<int> hist(10, 0);  
  for (int i = 0; i < 100000; i++) {
    hist[dist()]++;
  }

  int v = 0;
  for (auto p : hist) {
    std::cout << std::fixed << std::setprecision(1) << std::setw(2) << (v++)
              << ' ' << std::string(p / 500, '*') << " " << p << '\n';
  }
#else
  // Workload Distribution
  #define USE_UNIFORM_DISTRIBUTION 0
  #define USE_ZIPF_DISTRIBUTION    1

  const uint32_t num_threads_ =
    test::MultiThreadTestUtil::HardwareConcurrency() + (test::MultiThreadTestUtil::HardwareConcurrency() % 2);

  // This defines the key space (0 ~ (1M - 1))
  const uint32_t key_num = 1024 * 1024;
  std::atomic<size_t> insert_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  // Inserts in a 1M key space randomly until all keys has been inserted
  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    #if USE_UNIFORM_DISTRIBUTION
    util::Distribution<util::DistributionType::Uniform> dist(0, key_num - 1);
    #elif USE_ZIPF_DISTRIBUTION
    util::Distribution<util::DistributionType::Zipf> dist(0, key_num - 1, alpha);
    #endif

    dist.SetSeed(id);

    uint32_t op_cnt = 0;

    while (insert_success_counter.load() < key_num) {
      int key = dist();

      if (tree->Insert(key, key)) insert_success_counter.fetch_add(1);
      op_cnt++;
    }
    tree->UnregisterThread(gcid);
    total_op_counter.fetch_add(op_cnt);
  };

  util::Timer<std::milli> timer;
  timer.Start();

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, workload);
  tree->UpdateThreadLocal(1);

  timer.Stop();

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = insert_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "1M Insert(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

  timer.Start();
  // Verifies whether random insert is correct
  for (uint32_t i = 0; i < key_num; i++) {
    auto s = tree->GetValue(i);

    assert(s.size() == 1);
    assert(*s.begin() == i);
  }
  timer.Stop();

  double latency =  (timer.GetElapsed() / key_num);
  std::cout << std::fixed << "1M Get(): " << timer.GetElapsed() << " (ms), "
    << "avg read latency: " << latency << " (ms) " << std::endl;

  delete tree;
#endif
  return 0;
}
