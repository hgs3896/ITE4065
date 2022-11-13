#include "bwtree.h"
#include "bwtree_test_util.h"
#include "multithread_test_util.h"
#include "worker_pool.h"
#include "timer.h"
#include "distribution.h"
#include "custom_utils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>
#include <string>
#include <unordered_map>

/*******************************************************************************
 * The test structures stated here were written to give you and idea of what a
 * test should contain and look like. Feel free to change the code and add new
 * tests of your own. The more concrete your tests are, the easier it'd be to
 * detect bugs in the future projects.
 ******************************************************************************/

/*
 * TestFixture for bwtree tests
 */
class UtilTest : public ::testing::Test {
  public:
    static constexpr int min_value = 1;
    static constexpr int max_value = 9;
    static constexpr int nSamples = 1000;
    static constexpr int nVisualizationUnit = 10;
  protected:
    /*
     * NOTE: You can also use constructor/destructor instead of SetUp() and
     * TearDown(). The official document says that the former is actually
     * perferred due to some reasons. Checkout the document for the difference.
     */
    UtilTest() = default;
    ~UtilTest() = default;
};

TEST(UtilTest, UniformDist_SeedTest){
  util::Distribution<util::DistributionType::Uniform> dist_0(UtilTest::min_value, UtilTest::max_value);
  util::Distribution<util::DistributionType::Uniform> dist_1(UtilTest::min_value, UtilTest::max_value);
  
  dist_0.SetSeed(17);
  dist_1.SetSeed(31);
  for (int i = 0; i < 10; i++) {
    auto by_dist_0 = std::tuple<int, int, int>{dist_0(), dist_0(), dist_0()};
    auto by_dist_1 = std::tuple<int, int, int>{dist_1(), dist_1(), dist_1()};
    EXPECT_NE(by_dist_0, by_dist_1);
  }

  dist_0.SetSeed(17);
  dist_1.SetSeed(17);
  for (int i = 0; i < 10; i++) {
    auto by_dist_0 = std::tuple<int, int, int>{dist_0(), dist_0(), dist_0()};
    auto by_dist_1 = std::tuple<int, int, int>{dist_1(), dist_1(), dist_1()};
    EXPECT_EQ(by_dist_0, by_dist_1);
  }
}

TEST(UtilTest, UniformDistTest){
  util::Distribution<util::DistributionType::Uniform> dist(UtilTest::min_value, UtilTest::max_value);
  std::vector<int> hist(UtilTest::max_value - UtilTest::min_value + 1, 0);
  for (int i = 0; i < UtilTest::nSamples; i++) {
    hist[dist()-UtilTest::min_value]++;
  }

  int v = UtilTest::min_value;
  for (auto p : hist) {
    std::cout << std::fixed << std::setprecision(1) << std::setw(2) << (v++)
              << ' ' << std::string(p / UtilTest::nVisualizationUnit, '*') << " " << p << '\n';
  }
}

TEST(UtilTest, ZipfDist_SeedTest){
  util::Distribution<util::DistributionType::Zipf> dist_0(UtilTest::min_value, UtilTest::max_value, 1.0);
  util::Distribution<util::DistributionType::Zipf> dist_1(UtilTest::min_value, UtilTest::max_value, 1.0);
  dist_0.SetSeed(17);
  dist_1.SetSeed(31);
  for (int i = 0; i < 10; i++) {
    auto by_dist_0 = std::tuple<int, int, int>{dist_0(), dist_0(), dist_0()};
    auto by_dist_1 = std::tuple<int, int, int>{dist_1(), dist_1(), dist_1()};
    EXPECT_NE(by_dist_0, by_dist_1);
  }

  dist_0.SetSeed(17);
  dist_1.SetSeed(17);
  for (int i = 0; i < 10; i++) {
    auto by_dist_0 = std::tuple<int, int, int>{dist_0(), dist_0(), dist_0()};
    auto by_dist_1 = std::tuple<int, int, int>{dist_1(), dist_1(), dist_1()};
    EXPECT_EQ(by_dist_0, by_dist_1);
  }
}

TEST(UtilTest, ZipfDistTest_0){
  util::Distribution<util::DistributionType::Zipf> dist(UtilTest::min_value, UtilTest::max_value, 0.0);
  std::vector<int> hist(UtilTest::max_value - UtilTest::min_value + 1, 0);
  dist.SetSeed(0);
  for (int i = 0; i < UtilTest::nSamples; i++) {
    hist[dist()-UtilTest::min_value]++;
  }

  int v = UtilTest::min_value;
  for (auto p : hist) {
    std::cout << std::fixed << std::setprecision(1) << std::setw(2) << (v++)
              << ' ' << std::string(p / UtilTest::nVisualizationUnit, '*') << " " << p << '\n';
  }
}

TEST(UtilTest, ZipfDistTest_1){
  util::Distribution<util::DistributionType::Zipf> dist(UtilTest::min_value, UtilTest::max_value, 1.0);
  std::vector<int> hist(UtilTest::max_value - UtilTest::min_value + 1, 0);
  
  dist.SetSeed(0);
  for (int i = 0; i < UtilTest::nSamples; i++) {
    hist[dist()-UtilTest::min_value]++;
  }

  int v = UtilTest::min_value;
  for (auto p : hist) {
    std::cout << std::fixed << std::setprecision(1) << std::setw(2) << (v++)
              << ' ' << std::string(p / UtilTest::nVisualizationUnit, '*') << " " << p << '\n';
  }
}

TEST(UtilTest, ZipfDistTest_2){
  util::Distribution<util::DistributionType::Zipf> dist(UtilTest::min_value, UtilTest::max_value, 2.0);
  std::vector<int> hist(UtilTest::max_value - UtilTest::min_value + 1, 0);
  
  dist.SetSeed(0);
  for (int i = 0; i < UtilTest::nSamples; i++) {
    hist[dist()-UtilTest::min_value]++;
  }

  int v = UtilTest::min_value;
  for (auto p : hist) {
    std::cout << std::fixed << std::setprecision(1) << std::setw(2) << (v++)
              << ' ' << std::string(p / UtilTest::nVisualizationUnit, '*') << " " << p << '\n';
  }
}

/*
 * Tests bwtree init/destroy APIs.
 */
TEST(BwtreeInitTest, HandlesInitialization) {
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  ASSERT_TRUE(tree != nullptr);

  delete tree;
}

/*
 * TestFixture for bwtree tests
 */
class BwtreeTest : public ::testing::Test {
  public:
    static constexpr int KEY_NUM = 1024 * 8;
  protected:
    /*
     * NOTE: You can also use constructor/destructor instead of SetUp() and
     * TearDown(). The official document says that the former is actually
     * perferred due to some reasons. Checkout the document for the difference.
     */
    BwtreeTest() {
    }

    ~BwtreeTest() {
    }

    const uint32_t num_threads_ =
      test::MultiThreadTestUtil::HardwareConcurrency() + (test::MultiThreadTestUtil::HardwareConcurrency() % 2);
};

/*
 * Basic functionality test of different values insertions with the same key
 */
TEST_F(BwtreeTest, NonUniqueInsert) {
  // This defines the key space (0 ~ (1M - 1))
  const uint32_t key_num = 1024 * 1024;
  std::atomic<size_t> insert_success_counter = 0;

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  // Inserts in a 1M key space randomly until all keys has been inserted
  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);
    std::default_random_engine thread_generator(id);
    std::uniform_int_distribution<int> uniform_dist(0, key_num - 1);

    for(int key = 0; key < key_num; ++key){
      if (tree->Insert(key, key_num * id + key)) insert_success_counter.fetch_add(1);
    }
    
    tree->UnregisterThread(gcid);
  };

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, workload);
  tree->UpdateThreadLocal(1);

  // Verifies whether the insertion is correct
  for (uint32_t i = 0; i < key_num; i++) {
    auto s = tree->GetValue(i);

    EXPECT_EQ(s.size(), num_threads_);
    for(auto v : s){
      EXPECT_EQ(v % key_num, i);
    }
  }

  delete tree;
}

/*
 * Basic functionality test of different values insertions with the same key
 */
TEST_F(BwtreeTest, UniqueInsert) {
  // This defines the key space (0 ~ (1M - 1))
  const uint32_t key_num = 1024 * 1024;
  std::atomic<size_t> insert_success_counter = 0;

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  // Inserts in a 1M key space randomly until all keys has been inserted
  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);
    std::default_random_engine thread_generator(id);
    std::uniform_int_distribution<int> uniform_dist(0, key_num - 1);

    for(int key = 0; key < key_num; ++key){
      auto new_key = key_num * id + key;
      if (tree->Insert(new_key, new_key, true)) insert_success_counter.fetch_add(1);
    }
    
    tree->UnregisterThread(gcid);
  };

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, workload);
  tree->UpdateThreadLocal(1);

  // Verifies whether the insertion is correct
  for (uint32_t i = 0; i < num_threads_ * key_num; i++) {
    auto s = tree->GetValue(i);
    EXPECT_EQ(s.size(), 1);
  }

  delete tree;
}

/*
 * Basic functionality test of 1M concurrent random inserts
 */
TEST_F(BwtreeTest, ConcurrentRandomInsert) {
  // This defines the key space (0 ~ (1M - 1))
  const uint32_t key_num = 1024 * 1024;
  std::atomic<size_t> insert_success_counter = 0;

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  // Inserts in a 1M key space randomly until all keys has been inserted
  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);
    std::default_random_engine thread_generator(id);
    std::uniform_int_distribution<int> uniform_dist(0, key_num - 1);

    while (insert_success_counter.load() < key_num) {
      int key = uniform_dist(thread_generator);

      if (tree->Insert(key, key)) insert_success_counter.fetch_add(1);
    }
    tree->UnregisterThread(gcid);
  };

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, workload);
  tree->UpdateThreadLocal(1);

  // Verifies whether random insert is correct
  for (uint32_t i = 0; i < key_num; i++) {
    auto s = tree->GetValue(i);

    EXPECT_EQ(s.size(), 1);
    EXPECT_EQ(*s.begin(), i);
  }

  delete tree;
}

TEST_F(BwtreeTest, ConcurrentMixed) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (1M - 1))
  const uint32_t key_num = 1024 * 1024;

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);
    if ((id % 2) == 0) {
      for (uint32_t i = 0; i < key_num; i++) {
        int key = num_threads_ * i + id;  // NOLINT

        tree->Insert(key, key);
      }
    } else {
      for (uint32_t i = 0; i < key_num; i++) {
        int key = num_threads_ * i + id - 1;  // NOLINT

        while (!tree->Delete(key, key)) {
        }
      }
    }
    tree->UnregisterThread(gcid);
  };

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, workload);
  tree->UpdateThreadLocal(1);

  // Verifies that all values are deleted after mixed test
  for (uint32_t i = 0; i < key_num * num_threads_; i++) {
    EXPECT_EQ(tree->GetValue(i).size(), 0);
  }

  delete tree;
}

/*
 * Basic functionality test of 8K concurrent random skewed inserts
 */
/*
TEST_F(BwtreeTest, ConcurrentUniformInsert) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (8K - 1))
  const uint32_t key_num = KEY_NUM;
  std::atomic<size_t> insert_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  // Make skewed insert workload
  std::vector<std::vector<int>> key_lists(num_threads_);
  std::unordered_map<int, int> key_hist;
  for(uint32_t i = 0; i < num_threads_; i++) {
    key_lists[i].reserve(key_num);
    util::Distribution<util::DistributionType::Uniform> dist(0, key_num - 1);
    dist.SetSeed(i);
    for(uint32_t j = 0; j < key_num; j++) {
      key_lists[i].push_back(dist());
      ++key_hist[key_lists[i].back()];
    }
  }

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    uint32_t op_cnt = 0;
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      if(tree->Insert(key, key_num * id + i)) insert_success_counter.fetch_add(1);
      ++op_cnt;
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

  // Verifies that all values are inserted after insertion test
  for (auto [i, cnt] : key_hist) {
    auto item = tree->GetValue(i);
    EXPECT_EQ(item.size(), cnt);
  }

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = insert_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "8K Insert(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

#ifdef BWTREE_DEBUG
  std::cout << "# of Total Insert Operations: " << tree->insert_op_count.load()
            << std::endl
            << "# of Aborted Insert Operations: "
            << tree->insert_abort_count.load() << std::endl
            << "CAS Failure Per Insertion: "
            << (tree->insert_abort_count.load()) /
                   double(tree->insert_op_count.load())
            << std::endl;
#endif

  delete tree;
}
*/

/*
 * Basic functionality test of 8K concurrent random skewed inserts
 */
TEST_F(BwtreeTest, ConcurrentSkewedInsert_00) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (8K - 1))
  const uint32_t key_num = KEY_NUM;
  std::atomic<size_t> insert_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  // Make skewed insert workload
  std::vector<std::vector<int>> key_lists(num_threads_);
  std::unordered_map<int, int> key_hist;
  for(uint32_t i = 0; i < num_threads_; i++) {
    key_lists[i].reserve(key_num);
    util::Distribution<util::DistributionType::Zipf> dist(0, key_num - 1, 0.0);
    dist.SetSeed(i);
    for(uint32_t j = 0; j < key_num; j++) {
      key_lists[i].push_back(dist());
      ++key_hist[key_lists[i].back()];
    }
  }

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    uint32_t op_cnt = 0;
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      if(tree->Insert(key, key_num * id + i)) insert_success_counter.fetch_add(1);
      ++op_cnt;
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

  // Verifies that all values are inserted after insertion test
  for (auto [i, cnt] : key_hist) {
    auto item = tree->GetValue(i);
    EXPECT_EQ(item.size(), cnt);
  }

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = insert_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "8K Insert(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

#ifdef BWTREE_DEBUG
  std::cout << "# of Total Insert Operations: " << tree->insert_op_count.load()
            << std::endl
            << "# of Aborted Insert Operations: "
            << tree->insert_abort_count.load() << std::endl
            << "CAS Failure Per Insertion: "
            << (tree->insert_abort_count.load()) /
                   double(tree->insert_op_count.load())
            << std::endl;
#endif

  delete tree;
}

/*
 * Basic functionality test of 8K concurrent random skewed inserts
 */
TEST_F(BwtreeTest, ConcurrentSkewedInsert_10) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (8K - 1))
  const uint32_t key_num = KEY_NUM;
  std::atomic<size_t> insert_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  // Make skewed insert workload
  std::vector<std::vector<int>> key_lists(num_threads_);
  std::unordered_map<int, int> key_hist;
  for(uint32_t i = 0; i < num_threads_; i++) {
    key_lists[i].reserve(key_num);
    util::Distribution<util::DistributionType::Zipf> dist(0, key_num - 1, 1.0);
    dist.SetSeed(i);
    for(uint32_t j = 0; j < key_num; j++) {
      key_lists[i].push_back(dist());
      ++key_hist[key_lists[i].back()];
    }
  }

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    uint32_t op_cnt = 0;
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      if(tree->Insert(key, key_num * id + i)) insert_success_counter.fetch_add(1);
      ++op_cnt;
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

  // Verifies that all values are inserted after insertion test
  for (auto [i, cnt] : key_hist) {
    auto item = tree->GetValue(i);
    EXPECT_EQ(item.size(), cnt);
  }

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = insert_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "8K Insert(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

#ifdef BWTREE_DEBUG
  std::cout << "# of Total Insert Operations: " << tree->insert_op_count.load()
            << std::endl
            << "# of Aborted Insert Operations: "
            << tree->insert_abort_count.load() << std::endl
            << "CAS Failure Per Insertion: "
            << (tree->insert_abort_count.load()) /
                   double(tree->insert_op_count.load())
            << std::endl;
#endif

  delete tree;
}

/*
 * Basic functionality test of 8K concurrent random skewed inserts
 */
TEST_F(BwtreeTest, ConcurrentSkewedInsert_20) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (8K - 1))
  const uint32_t key_num = KEY_NUM;
  std::atomic<size_t> insert_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  // Make skewed insert workload
  std::vector<std::vector<int>> key_lists(num_threads_);
  std::unordered_map<int, int> key_hist;
  for(uint32_t i = 0; i < num_threads_; i++) {
    key_lists[i].reserve(key_num);
    util::Distribution<util::DistributionType::Zipf> dist(0, key_num - 1, 2.0);
    dist.SetSeed(i);
    for(uint32_t j = 0; j < key_num; j++) {
      key_lists[i].push_back(dist());
      ++key_hist[key_lists[i].back()];
    }
  }

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    uint32_t op_cnt = 0;
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      if(tree->Insert(key, key_num * id + i)) insert_success_counter.fetch_add(1);
      ++op_cnt;
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

  // Verifies that all values are inserted after insertion test
  for (auto [i, cnt] : key_hist) {
    auto item = tree->GetValue(i);
    EXPECT_EQ(item.size(), cnt);
  }

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = insert_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "8K Insert(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

#ifdef BWTREE_DEBUG
  std::cout << "# of Total Insert Operations: " << tree->insert_op_count.load()
            << std::endl
            << "# of Aborted Insert Operations: "
            << tree->insert_abort_count.load() << std::endl
            << "CAS Failure Per Insertion: "
            << (tree->insert_abort_count.load()) /
                   double(tree->insert_op_count.load())
            << std::endl;
#endif

  delete tree;
}

/*
 * Basic functionality test of 8K concurrent random skewed deletes
 */
TEST_F(BwtreeTest, ConcurrentSkewedDelete_00) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (8K - 1))
  const uint32_t key_num = KEY_NUM;
  std::atomic<size_t> delete_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  // Make skewed insert workload
  std::vector<std::vector<int>> key_lists(num_threads_);
  std::unordered_map<int, int> key_hist;
  for(uint32_t i = 0; i < num_threads_; i++) {
    key_lists[i].reserve(key_num);
    util::Distribution<util::DistributionType::Zipf> dist(0, key_num - 1, 0.0);
    dist.SetSeed(i);
    for(uint32_t j = 0; j < key_num; j++) {
      key_lists[i].push_back(dist());
      ++key_hist[key_lists[i].back()];
    }
  }

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto insert_workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      tree->Insert(key, key_num * id + i);
    }

    tree->UnregisterThread(gcid);
  };

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, insert_workload);
  tree->UpdateThreadLocal(1);

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    uint32_t op_cnt = 0;
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      if(tree->Delete(key, key_num * id + i)) delete_success_counter.fetch_add(1);
      ++op_cnt;
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

  // Verifies that all values are inserted after insertion test
  for (auto [i, cnt] : key_hist) {
    auto item = tree->GetValue(i);
    EXPECT_EQ(item.size(), 0);
  }

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = delete_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "8K Delete(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

#ifdef BWTREE_DEBUG
  std::cout << "# of Total Delete Operations: " << tree->delete_op_count.load()
            << std::endl
            << "# of Aborted Delete Operations: "
            << tree->delete_abort_count.load() << std::endl
            << "CAS Failure Per Deletion: "
            << (tree->delete_abort_count.load()) /
                   double(tree->delete_op_count.load())
            << std::endl;
#endif

  delete tree;
}

/*
 * Basic functionality test of 8K concurrent random skewed deletes
 */
TEST_F(BwtreeTest, ConcurrentSkewedDelete_10) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (8K - 1))
  const uint32_t key_num = KEY_NUM;
  std::atomic<size_t> delete_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  // Make skewed insert workload
  std::vector<std::vector<int>> key_lists(num_threads_);
  std::unordered_map<int, int> key_hist;
  for(uint32_t i = 0; i < num_threads_; i++) {
    key_lists[i].reserve(key_num);
    util::Distribution<util::DistributionType::Zipf> dist(0, key_num - 1, 1.0);
    dist.SetSeed(i);
    for(uint32_t j = 0; j < key_num; j++) {
      key_lists[i].push_back(dist());
      ++key_hist[key_lists[i].back()];
    }
  }

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto insert_workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      tree->Insert(key, key_num * id + i);
    }

    tree->UnregisterThread(gcid);
  };

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, insert_workload);
  tree->UpdateThreadLocal(1);

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    uint32_t op_cnt = 0;
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      if(tree->Delete(key, key_num * id + i)) delete_success_counter.fetch_add(1);
      ++op_cnt;
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

  // Verifies that all values are inserted after insertion test
  for (auto [i, cnt] : key_hist) {
    auto item = tree->GetValue(i);
    EXPECT_EQ(item.size(), 0);
  }

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = delete_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "8K Delete(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

#ifdef BWTREE_DEBUG
  std::cout << "# of Total Delete Operations: " << tree->delete_op_count.load()
            << std::endl
            << "# of Aborted Delete Operations: "
            << tree->delete_abort_count.load() << std::endl
            << "CAS Failure Per Deletion: "
            << (tree->delete_abort_count.load()) /
                   double(tree->delete_op_count.load())
            << std::endl;
#endif

  delete tree;
}

/*
 * Basic functionality test of 8K concurrent random skewed deletes
 */
TEST_F(BwtreeTest, ConcurrentSkewedDelete_20) {
  NOISEPAGE_ASSERT(num_threads_ % 2 == 0,
      "This test requires an even number of threads. This should have been handled when it was assigned.");

  // This defines the key space (0 ~ (8K - 1))
  const uint32_t key_num = KEY_NUM;
  std::atomic<size_t> delete_success_counter = 0;
  std::atomic<size_t> total_op_counter = 0;

  // Make skewed insert workload
  std::vector<std::vector<int>> key_lists(num_threads_);
  std::unordered_map<int, int> key_hist;
  for(uint32_t i = 0; i < num_threads_; i++) {
    key_lists[i].reserve(key_num);
    util::Distribution<util::DistributionType::Zipf> dist(0, key_num - 1, 2.0);
    dist.SetSeed(i);
    for(uint32_t j = 0; j < key_num; j++) {
      key_lists[i].push_back(dist());
      ++key_hist[key_lists[i].back()];
    }
  }

  common::WorkerPool thread_pool(num_threads_, {});
  thread_pool.Startup();
  auto *const tree = test::BwTreeTestUtil::GetEmptyTree();

  auto insert_workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      tree->Insert(key, key_num * id + i);
    }

    tree->UnregisterThread(gcid);
  };

  tree->UpdateThreadLocal(num_threads_ + 1);
  test::MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, num_threads_, insert_workload);
  tree->UpdateThreadLocal(1);

  auto workload = [&](uint32_t id) {
    const uint32_t gcid = id + 1;
    tree->AssignGCID(gcid);

    uint32_t op_cnt = 0;
    
    for (uint32_t i = 0; i < key_num; i++) {
      int key = key_lists[id][i];

      if(tree->Delete(key, key_num * id + i)) delete_success_counter.fetch_add(1);
      ++op_cnt;
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

  // Verifies that all values are inserted after insertion test
  for (auto [i, cnt] : key_hist) {
    auto item = tree->GetValue(i);
    EXPECT_EQ(item.size(), 0);
  }

  double ops = total_op_counter.load() / (timer.GetElapsed() / 1000.0);
  double success_ops = delete_success_counter.load() / (timer.GetElapsed() / 1000.0);
  std::cout << std::fixed << "8K Delete(): " << timer.GetElapsed() << " (ms), "
    << "write throughput: " << ops << " (op/s), "
    << "successive write throughput: " << success_ops << " (op/s)" << std::endl;

#ifdef BWTREE_DEBUG
  std::cout << "# of Total Delete Operations: " << tree->delete_op_count.load()
            << std::endl
            << "# of Aborted Delete Operations: "
            << tree->delete_abort_count.load() << std::endl
            << "CAS Failure Per Deletion: "
            << (tree->delete_abort_count.load()) /
                   double(tree->delete_op_count.load())
            << std::endl;
#endif

  delete tree;
}