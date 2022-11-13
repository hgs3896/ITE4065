# 3. Improve the Bw-tree



## 1. Motivation

**Open Bw-tree** is a lock-free B-tree redesigned by Ziqi Wang et. al., 2018 to achieve cache friendliness and high concurrency for new hardware platforms such as multicore chips. The original version of Bw-tree was developed by Microsoft Research, although it lacks some important details and its public implementation.

To understand the design principles of the Open Bw-tree and search a way to improve it through scientific analysis,

1. Design various workloads to analyze the performance metrics of each workload
2. Benchmark the current Open Bw-tree on various workloads
3. Improve the Open Bw-tree according to the analysis



## 2. Benchmark of the current Bw-tree

> Test Environment
>
> 
>
> OS: Linux multicore-20 5.4.0-121-generic #137-Ubuntu
>
> CPU: Intel(R) Xeon(R) CPU E5-2630 v4 @ 2.20GHz (20 Cores = (Cores per socket = 10) * (# of sockets = 2))
>
> - L1d cache: 640 KiB
> - L1i cache: 640 KiB
> - L2 cache: 5 MiB
> - L3 cache: 50 MiB
>
> Memory: 
>
> gcc: gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1)
>
> make: GNU Make 4.2.1
>
> cmake: cmake version 3.16.3
>
> perf: perf version 5.4.210



To understand which workload affects more critically to the performance of insertions and deletions of Bw-tree, I first propose a set of skewed workloads using **Zipf distribution generator** provided in the sample Bw-tree code.

Then, I measured the various metrics such as the latency/throughput of an operation, the number of successful/failed **CAS(Compare-and-Swap)** operations, and the ratio of failed **CAS** with respect to successful CAS operations with each workload.

By scrutinizing the metrics, I found the main bottleneck for Open Bw-tree could be **CAS operations** being performed on a few of **hot mapping table entries**, since the latency and ratio of CAS failure per insertion operation skyrockets as the skewness increases and it might lead to a huge amount of cache invalidation storms, which delays the whole insert progress. The same reason can be applied to the deletion, but it is less affected than that of the insertion.



### Insert Only Workload

> Breif Explanation of Insert Only Workload
>
> - 8K insertions per each thread
> - 20 Threads
>
> Total 160K Insertions are made into the Open Bw-tree
>
> To avoid the effect of generating random keys and values, all keys and values are pregenerated, stored into a thread local vector and then incrementally fetched by each thread later

![Insert Only Workload Plot](original_openbwtree_insert_performance.png)

|                Metric                 | Uniform Workload<br />(Zipf: alpha=0.0) | Slightly Skewed Workload<br />(Zipf: alpha=1.0) | Skewed Workload<br />(Zipf: alpha=2.0) |
| :-----------------------------------: | :-------------------------------------: | :---------------------------------------------: | :------------------------------------: |
|       8K Insert() Latency (ms)        |                  73.0                   |                     2981.8                      |                111771.3                |
|        write throughput (op/s)        |                2244254.1                |                     54946.2                     |                 1465.9                 |
|  successive write throughput (op/s)   |                2244254.1                |                     54946.2                     |                 1465.9                 |
|  \# of Total Insert Operations (ops)  |                163840.0                 |                    163840.0                     |                163840.0                |
| \# of Aborted Insert Operations (ops) |                 4242.0                  |                    271214.0                     |               1592268.0                |
|   CAS Failure Per Insertion (ratio)   |           0.0<br />(Rounded)            |                       1.7                       |                  9.7                   |



## Delete Only Workload

> Breif Explanation of Delete Only Workload
>
> To be deleted, insert all required keys in the tree
>
> - 160K insertions are first made into the Bw-tree
>
> To test the deletion,
>
> - 8K deletions per each thread
> - 20 Threads
>
> Total 160K deletions are made into the Open Bw-tree
>
> To avoid the effect of generating random keys and values, all keys and values are pregenerated, stored into a thread local vector and then incrementally fetched by each thread later

![Delete Only Workload Plot](original_openbwtree_delete_performance.png)

|                Metric                | Uniform Workload<br />(Zipf: alpha=0.0) | Slightly Skewed Workload<br />(Zipf: alpha=1.0) | Skewed Workload<br />(Zipf: alpha=2.0) |
| :----------------------------------: | :-------------------------------------: | :---------------------------------------------: | :------------------------------------: |
|       8K Delete() Latency (ms)       |                  71.6                   |                      733.8                      |                20983.0                 |
|       write throughput (op/s)        |                2288468.9                |                    223283.4                     |                 7808.2                 |
|  successive write throughput (op/s)  |                2288468.9                |                    223283.4                     |                 7808.2                 |
|  # of Total Delete Operations (ops)  |                163840.0                 |                    163840.0                     |                163840.0                |
| # of Aborted Delete Operations (ops) |                 1214.0                  |                     64448.0                     |                271787.0                |
|   CAS Failure Per Deletion (ratio)   |           0.0<br />(Rounded)            |                       0.4                       |                  1.7                   |



To check whether the main performance collapse for skewed workload is due to **hot CAS operations**, I tried **perf-c2c** to measure cache statistics of the proposed workload.

As the skewness goes higher, the total number of **HITM(Cache HIT Modified)** samples arises in both insertion-only and deletion-only workload.

> The **HITM** is an event that it needs to wait for reading the invalidated cache due to modification.
>
> **Local HITM** occurs where the cache is invalidated within the socket.
> However, **remote HITM** happens where the cache is invalidated outside the current CPU socket, which is critical to the performance and requires more cycles to get the cache valid.



#### Insert Only Workload

| Load HITM<br /># of Samples | Uniform Workload<br />(Zipf: alpha=0.0) | Slightly Skewed Workload<br />(Zipf: alpha=1.0) | Skewed Workload<br />(Zipf: alpha=2.0) |
| :-------------------------: | :-------------------------------------: | :---------------------------------------------: | :------------------------------------: |
|         Local HITM          |                   22                    |                       97                        |                  231                   |
|         Remote HITM         |                   17                    |                       224                       |                  1988                  |
|         Total HITM          |                   39                    |                       321                       |                  2219                  |



#### Delete Only Workload

| Load HITM<br /># of Samples | Uniform Workload<br />(Zipf: alpha=0.0) | Slightly Skewed Workload<br />(Zipf: alpha=1.0) | Skewed Workload<br />(Zipf: alpha=2.0) |
| :-------------------------: | :-------------------------------------: | :---------------------------------------------: | :------------------------------------: |
|         Local HITM          |                   52                    |                       709                       |                 37655                  |
|         Remote HITM         |                   119                   |                      1122                       |                 44857                  |
|         Total HITM          |                   171                   |                      1831                       |                 82512                  |



Further details are in project3/perf_results/.

Thus, the cache invalidation is the main performance bottleneck for skewed workload.



## 3. Problem Definition

Frequent cache invalidation is the key to solve the CAS performance on mapping table entries.



## 4. Solution

In the current design, **CAS**ing to the mapping table is done <u>without cooperation</u>.

By **cooperating** with each thread, the number of **operation retries** can be done in <u>a finite steps</u> with the help of other thread.

Introducing **the announce array approach**, the lock-free installation of a node into mapping table in insertion and deletion can be done in a wait-free manner.

1. Make **head** and **announce** arrays with each size **n**(the number of threads)
2. Initialize the entries of the head and announce arrays with a pointer to Base LeafNode which has a sequence number 1.
3. Let i be the currrent thread's id.
   Then, announce[i] = (<u>a node to be installed</u>)
4. head[i] = (a pointer to BaseNode whose has the latest sequence number in **head** array)
5. Until announce[i].seq is not 0, do the same logic in the text book.
   (The consensus can be implemented also using **TAS**(Test-and-Set) too.)
6. Finish the installation



# Full Bw-tree Benchmark Results

```
./debug/bin/bwtree_test
Running main() from /home/2022_cp_project/student30473/ITE4065-main/project3/debug/_deps/googletest-src/googletest/src/gtest_main.cc
[==========] Running 17 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 6 tests from UtilTest
[ RUN      ] UtilTest.UniformDist_SeedTest
[       OK ] UtilTest.UniformDist_SeedTest (0 ms)
[ RUN      ] UtilTest.UniformDistTest
 1 ********** 106
 2 ********** 109
 3 ********** 102
 4 *********** 118
 5 ************* 133
 6 ********** 103
 7 ************* 133
 8 ********** 100
 9 ********* 96
[       OK ] UtilTest.UniformDistTest (0 ms)
[ RUN      ] UtilTest.ZipfDist_SeedTest
[       OK ] UtilTest.ZipfDist_SeedTest (0 ms)
[ RUN      ] UtilTest.ZipfDistTest_0
 1 ********** 105
 2 ************* 134
 3 ********** 101
 4 *********** 110
 5 *********** 118
 6 *********** 110
 7 *********** 119
 8 ********** 100
 9 ********** 103
[       OK ] UtilTest.ZipfDistTest_0 (0 ms)
[ RUN      ] UtilTest.ZipfDistTest_1
 1 *********************************** 357
 2 ****************** 186
 3 *********** 112
 4 ********* 97
 5 ****** 65
 6 ****** 62
 7 **** 42
 8 **** 45
 9 *** 34
[       OK ] UtilTest.ZipfDistTest_1 (0 ms)
[ RUN      ] UtilTest.ZipfDistTest_2
 1 ***************************************************************** 657
 2 **************** 161
 3 ******* 73
 4 *** 38
 5 ** 28
 6 * 17
 7 * 13
 8  9
 9  4
[       OK ] UtilTest.ZipfDistTest_2 (0 ms)
[----------] 6 tests from UtilTest (1 ms total)

[----------] 1 test from BwtreeInitTest
[ RUN      ] BwtreeInitTest.HandlesInitialization
[       OK ] BwtreeInitTest.HandlesInitialization (0 ms)
[----------] 1 test from BwtreeInitTest (0 ms total)

[----------] 10 tests from BwtreeTest
[ RUN      ] BwtreeTest.NonUniqueInsert
[       OK ] BwtreeTest.NonUniqueInsert (22754 ms)
[ RUN      ] BwtreeTest.UniqueInsert
[       OK ] BwtreeTest.UniqueInsert (53568 ms)
[ RUN      ] BwtreeTest.ConcurrentRandomInsert
[       OK ] BwtreeTest.ConcurrentRandomInsert (5400 ms)
[ RUN      ] BwtreeTest.ConcurrentMixed
[       OK ] BwtreeTest.ConcurrentMixed (28575 ms)
[ RUN      ] BwtreeTest.ConcurrentSkewedInsert_00
8K Insert(): 73.0 (ms), write throughput: 2244254.1 (op/s), successive write throughput: 2244254.1 (op/s)
# of Total Insert Operations: 163840
# of Aborted Insert Operations: 4242
CAS Failure Per Insertion: 0.0
[       OK ] BwtreeTest.ConcurrentSkewedInsert_00 (3510 ms)
[ RUN      ] BwtreeTest.ConcurrentSkewedInsert_10
8K Insert(): 2981.8 (ms), write throughput: 54946.2 (op/s), successive write throughput: 54946.2 (op/s)
# of Total Insert Operations: 163840
# of Aborted Insert Operations: 271214
CAS Failure Per Insertion: 1.7
[       OK ] BwtreeTest.ConcurrentSkewedInsert_10 (3872 ms)
[ RUN      ] BwtreeTest.ConcurrentSkewedInsert_20
8K Insert(): 111771.3 (ms), write throughput: 1465.9 (op/s), successive write throughput: 1465.9 (op/s)
# of Total Insert Operations: 163840
# of Aborted Insert Operations: 1592268
CAS Failure Per Insertion: 9.7
[       OK ] BwtreeTest.ConcurrentSkewedInsert_20 (111976 ms)
[ RUN      ] BwtreeTest.ConcurrentSkewedDelete_00
8K Delete(): 71.6 (ms), write throughput: 2288468.9 (op/s), successive write throughput: 2288468.9 (op/s)
# of Total Delete Operations: 163840
# of Aborted Delete Operations: 1214
CAS Failure Per Deletion: 0.0
[       OK ] BwtreeTest.ConcurrentSkewedDelete_00 (3537 ms)
[ RUN      ] BwtreeTest.ConcurrentSkewedDelete_10
8K Delete(): 733.8 (ms), write throughput: 223283.4 (op/s), successive write throughput: 223283.4 (op/s)
# of Total Delete Operations: 163840
# of Aborted Delete Operations: 64448
CAS Failure Per Deletion: 0.4
[       OK ] BwtreeTest.ConcurrentSkewedDelete_10 (5638 ms)
[ RUN      ] BwtreeTest.ConcurrentSkewedDelete_20
8K Delete(): 20983.0 (ms), write throughput: 7808.2 (op/s), successive write throughput: 7808.2 (op/s)
# of Total Delete Operations: 163840
# of Aborted Delete Operations: 271787
CAS Failure Per Deletion: 1.7
[       OK ] BwtreeTest.ConcurrentSkewedDelete_20 (150459 ms)
[----------] 10 tests from BwtreeTest (389293 ms total)

[----------] Global test environment tear-down
[==========] 17 tests from 3 test suites ran. (389295 ms total)
[  PASSED  ] 17 tests.
```

