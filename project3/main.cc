#include <algorithm>
#include <random>
#include <vector>
#include <string>
#include <iostream>

#include "bwtree.h"
#include "bwtree_test_util.h"
#include "multithread_test_util.h"
#include "timer.h"
#include "worker_pool.h"
#include "zipf.h"

int main(int argc, char *argv[]) {
#if 0
  double alpha = 3.0;
  std::vector<int> count(11, 0);

  rand_val(alpha);
  for (int i = 0; i < 10000; i++) {
    count[zipf(alpha, 10)]++;
  }

  std::sort(count.begin(), count.end());

  std::cout << "Ratio for alpha: " << alpha << std::endl;
  for (int i = 1; i < 11; i++) {
    std::cout << count[i] << std::endl;
  }
#endif
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
    std::default_random_engine thread_generator(id);
    std::uniform_int_distribution<int> uniform_dist(0, key_num - 1);
    uint32_t op_cnt = 0;

    while (insert_success_counter.load() < key_num) {
      int key = uniform_dist(thread_generator);

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

  return 0;
}
