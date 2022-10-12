# 2. Atomic Wait-free Snapshot

## What is an atomic wait-free snapshot

**An atomic wait-free snapshot** constructs an <u>instantaneous view</u> of **an array of atomic registers** in a <u>wait-free</u> manner.  
It can be used for real-time backups or checkpoints.



## How to compile and run

0. Go to project2 directory by `cd project2`
1. Compile and build the **project2** by `./compile.sh`
2. `./run (# of worker threads)` or `./build/AtomicSnapshot (# of worker threads)`



## How to test the Project 2?

0. Go to project2 directory by `cd project2`
1. For debug version (additional symbols included & not optimized)
   1. Compile and build the **project2** by `./compile_debug.sh`
   2. `./build/test/tester` to run the tests
2. For release version (optimized)
   1. Compile and build the **project2** by `compile_with_tests.sh`
   2. `./build/test/tester` to run the tests

The tests are written as GTests(Google Test).



## Design Summary

In order to implement a wait-free atomic snapshot,

1. It is required to construct the basic conceptual methods such as **`collect()`**, **`scan()`** and **`update()`**.

2. To check the number of updates made in each thread, I put **`getNumberOfUpdates()`** into the interface.

3. To save the thread creation costs, **reuse the thread pool implementation** used in the previous project. (`ThreadPool.hpp`)

4. To hold the snap value, make a struct named `WaitFreeAtomicSnapshotItem<T>`.

```c++
template<typename T>
struct WaitFreeAtomicSnapshotItem{
		T data;            // The value to hold
    uint64_t version;  // Item Version(Label)
    Snapshot<T>* last; // Last snapshot pointer after this item was written
}
```

5. For a snapshot to hold multiple snap values, make a struct named `struct Snapshot<T>`.

```c++
template<typename T>
struct Snapshot {
    WaitFreeAtomicSnapshotItem<T>* items[N]; // an array of pointers to snap value items
};
```

It should manage the array of snap values.

6. To get a clean collect and make an update to the array, `WaitFreeAtomicSnapshot` is to have these features.

```C++
template<typename T>
class WaitFreeAtomicSnapshot {
private:
    Snapshot<T>* collect() const;        // Skim an entire array and return it
public:
    void update(size_t tid, T new_data); // Update an tid'th item in an array
    Snapshot<T>* scan() const;           // Scan (clean collect) the array
}
```



## Implementation Detail

1. To get the freedom from directly managing heap memory, using **C++ smart pointers** supported by C++11 is one of my design choices.

- For `Snapshot<T>`, due to its exclusive access to only one thread, it usage is adequate to that of `std::unique_ptr<Snapshot<T>>`.

- For `WaitFreeAtomicSnapshotItem<T>`, it can be shared by multiple snapshots.
  Therefore, it should be handled by `std::shared_ptr<WaitFreeAtomicSnapshotItem<T>>`.

2. Deferred Deallocations
   - By using `std::unique_ptr<Snapshot<T>>` and `std::shared_ptr<WaitFreeAtomicSnapshotItem<T>>`, the linked structure of **snapshot objects** and **WaitFreeAtomicSnapshotItem objects** internally has a problem of **recursive destruction calls** in which the destruction stacks can be easily overflowed.
   - To avoid deep recursive destruction process, implement deferred deallocations not to overflow the stacks

3. To atomically switch the pointer, use `std::atomic_load()` and `std::atomic_store()`
   - Since shared_ptrs are not thread-safe, though the management of its control block is thread-safe, I use `std::atomic_load()` and `std::atomic_store()` to atomically load/store the pointer.



## Performance Analysis

![Release Binary Analysis](/Users/hgs/github/ParallelProgramming/ITE4065/project2/release.png)

This figure shows the relationship between **the number of updater threads** (in x-axis) and **the total number of updates** being made during <u>1 minute</u> (in y-axis) in **release binary**.

- As the graph clearly shows that the update performance is highest when the number of updaters is small. It is because the more updater threads, the harder to get a clean snapshot **due to a lot of changes being made by the updaters**.
  

![Debug Binary Analysis](/Users/hgs/github/ParallelProgramming/ITE4065/project2/debug.png)

This figure shows the relationship between **the number of updater threads** (in x-axis) and **the total number of updates** being made during <u>1 minute</u> (in y-axis) in **debug binary**.

- As you see, the release binary is much efficient than the debug binary, since its operations are optimized.
- The shape of the graph from the debug binary is slightly different from that of release binary, it is caused by some compiler optimization options such as `-Ofast`, `-g`, `-fsanitize=~~~`.