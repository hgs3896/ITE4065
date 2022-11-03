#include "bwtree.h"

namespace bwtree {

// This will be initialized when thread is initialized and in a per-thread
// basis, i.e. each thread will get the same initialization image and then
// is free to change them
thread_local int bwtree::BwTreeBase::gc_id = -1;

std::atomic<size_t> bwtree::BwTreeBase::total_thread_num{0UL};

}  // namespace bwtree
