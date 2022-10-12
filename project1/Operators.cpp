#include <Operators.hpp>
#include <ThreadPool.hpp>
#include <cassert>
#include <list>
#include <future>
#include <iostream>
#include <vector>
#include <iterator>
#include <numeric>

static ThreadPool pool_for_intraoperation(size_t(2.5*ThreadPool::GetSupportedHardwareThreads()));
ThreadPool pool_for_interoperation(ThreadPool::GetSupportedHardwareThreads()/2);

//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
bool Scan::require(SelectInfo info)
  // Require a column and add it to results
{
  if (info.binding!=relationBinding)
    return false;
  assert(info.colId<relation.columns.size());
  resultColumns.push_back(relation.columns[info.colId]);
  select2ResultColId[info]=resultColumns.size()-1;
  return true;
}
//---------------------------------------------------------------------------
void Scan::run()
  // Run
{
  // Nothing to do
  resultSize=relation.size;
}
//---------------------------------------------------------------------------
vector<uint64_t*> Scan::getResults()
  // Get materialized results
{
  return resultColumns;
}
//---------------------------------------------------------------------------
bool FilterScan::require(SelectInfo info)
  // Require a column and add it to results
{
  if (info.binding!=relationBinding)
    return false;
  assert(info.colId<relation.columns.size());
  if (select2ResultColId.find(info)==select2ResultColId.end()) {
    // Add to results
    inputData.push_back(relation.columns[info.colId]);
    tmpResults.emplace_back();
    unsigned colId=tmpResults.size()-1;
    select2ResultColId[info]=colId;
  }
  return true;
}
//---------------------------------------------------------------------------
void FilterScan::copy2Result(uint64_t id)
  // Copy to result
{
  for (unsigned cId=0;cId<inputData.size();++cId)
    tmpResults[cId].push_back(inputData[cId][id]);
  ++resultSize;
}
//---------------------------------------------------------------------------
bool FilterScan::applyFilter(uint64_t i,FilterInfo& f)
  // Apply filter
{
  auto compareCol=relation.columns[f.filterColumn.colId];
  auto constant=f.constant;
  switch (f.comparison) {
    case FilterInfo::Comparison::Equal:
      return compareCol[i]==constant;
    case FilterInfo::Comparison::Greater:
      return compareCol[i]>constant;
    case FilterInfo::Comparison::Less:
       return compareCol[i]<constant;
  };
  return false;
}
//---------------------------------------------------------------------------
void FilterScan::run()
  // Run
{
  for (uint64_t i=0;i<relation.size;++i) {
    bool pass=true;
    for (auto& f : filters) {
      pass&=applyFilter(i,f);
    }
    if (pass)
      copy2Result(i);
  }
}
//---------------------------------------------------------------------------
void ParallelFilterScan::run() {
  using RecordIndex = std::vector<uint64_t>;
  std::vector<std::future<RecordIndex>> jobs;
  const auto tuple_size = relation.columns.size() * sizeof(uint64_t);
  const auto tuples_per_job = L2CacheLineSize / tuple_size;
  const auto job_size = (relation.size + tuples_per_job - 1) / tuples_per_job;

  for (int i = 0; i < job_size; ++i) {
    auto s = i * tuples_per_job, e = min<size_t>((i + 1) * tuples_per_job, relation.size);
    if (s >= e)
      break;
    jobs.emplace_back(pool_for_intraoperation.EnqueueJob([this, s, e] {
      RecordIndex result;
      result.reserve(e-s);
      for (uint64_t i = s; i < e; ++i) {
        bool pass = true;
        for (auto &f : filters) {
          pass &= applyFilter(i, f);
        }
        if (pass) {
          result.emplace_back(i);
        }
      }
      return result;
    }));
  }
  for(auto&& job : jobs){
    for(auto i : job.get()){
      copy2Result(i);
    }
  }
}
//---------------------------------------------------------------------------
vector<uint64_t*> Operator::getResults()
  // Get materialized results
{
  vector<uint64_t*> resultVector;
  for (auto& c : tmpResults) {
    resultVector.push_back(c.data());
  }
  return resultVector;
}
//---------------------------------------------------------------------------
bool Join::require(SelectInfo info)
  // Require a column and add it to results
{
  if (requestedColumns.count(info)==0) {
    bool success=false;
    if(left->require(info)) {
      requestedColumnsLeft.emplace_back(info);
      success=true;
    } else if (right->require(info)) {
      success=true;
      requestedColumnsRight.emplace_back(info);
    }
    if (!success)
      return false;

    tmpResults.emplace_back();
    requestedColumns.emplace(info);
  }
  return true;
}
//---------------------------------------------------------------------------
void Join::copy2Result(uint64_t leftId,uint64_t rightId)
  // Copy to result
{
  unsigned relColId=0;
  for (unsigned cId=0;cId<copyLeftData.size();++cId)
    tmpResults[relColId++].push_back(copyLeftData[cId][leftId]);

  for (unsigned cId=0;cId<copyRightData.size();++cId)
    tmpResults[relColId++].push_back(copyRightData[cId][rightId]);
  ++resultSize;
}
//---------------------------------------------------------------------------
void Join::run()
  // Run
{
  left->require(pInfo.left);
  right->require(pInfo.right);
  left->run();
  right->run();

  // Use smaller input for build
  if (left->resultSize>right->resultSize) {
    swap(left,right);
    swap(pInfo.left,pInfo.right);
    swap(requestedColumnsLeft,requestedColumnsRight);
  }

  auto leftInputData=left->getResults();
  auto rightInputData=right->getResults();

  // Resolve the input columns
  unsigned resColId=0;
  for (auto& info : requestedColumnsLeft) {
    copyLeftData.push_back(leftInputData[left->resolve(info)]);
    select2ResultColId[info]=resColId++;
  }
  for (auto& info : requestedColumnsRight) {
    copyRightData.push_back(rightInputData[right->resolve(info)]);
    select2ResultColId[info]=resColId++;
  }

  auto leftColId=left->resolve(pInfo.left);
  auto rightColId=right->resolve(pInfo.right);

  // Build phase
  auto leftKeyColumn=leftInputData[leftColId];
  hashTable.reserve(left->resultSize*2);
  for (uint64_t i=0,limit=i+left->resultSize;i!=limit;++i) {
    hashTable.emplace(leftKeyColumn[i],i);
  }
  // Probe phase
  auto rightKeyColumn=rightInputData[rightColId];
  for (uint64_t i=0,limit=i+right->resultSize;i!=limit;++i) {
    auto rightKey=rightKeyColumn[i];
    auto range=hashTable.equal_range(rightKey);
    for (auto iter=range.first;iter!=range.second;++iter) {
      copy2Result(iter->second,i);
    }
  }
}
//---------------------------------------------------------------------------
void SelfJoin::copy2Result(uint64_t id)
  // Copy to result
{
  for (unsigned cId=0;cId<copyData.size();++cId)
    tmpResults[cId].push_back(copyData[cId][id]);
  ++resultSize;
}
//---------------------------------------------------------------------------
bool SelfJoin::require(SelectInfo info)
  // Require a column and add it to results
{
  if (requiredIUs.count(info))
    return true;
  if(input->require(info)) {
    tmpResults.emplace_back();
    requiredIUs.emplace(info);
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
void SelfJoin::run()
  // Run
{
  input->require(pInfo.left);
  input->require(pInfo.right);
  input->run();
  inputData=input->getResults();

  for (auto& iu : requiredIUs) {
    auto id=input->resolve(iu);
    copyData.emplace_back(inputData[id]);
    select2ResultColId.emplace(iu,copyData.size()-1);
  }

  auto leftColId=input->resolve(pInfo.left);
  auto rightColId=input->resolve(pInfo.right);

  auto leftCol=inputData[leftColId];
  auto rightCol=inputData[rightColId];
  for (uint64_t i=0;i<input->resultSize;++i) {
    if (leftCol[i]==rightCol[i])
      copy2Result(i);
  }
}
//---------------------------------------------------------------------------
void Checksum::run()
  // Run
{
  for (auto& sInfo : colInfo) {
    input->require(sInfo);
  }
  input->run();
  auto results=input->getResults();

  for (auto& sInfo : colInfo) {
    auto colId=input->resolve(sInfo);
    auto resultCol=results[colId];
    uint64_t sum=0;
    resultSize=input->resultSize;
    for (auto iter=resultCol,limit=iter+input->resultSize;iter!=limit;++iter)
      sum+=*iter;
    checkSums.push_back(sum);
  }
}
//---------------------------------------------------------------------------
bool ParallelHashJoin::require(SelectInfo info)
  // Require a column and add it to results
{
  if (requestedColumns.count(info)==0) {
    bool success=false;
    if(left->require(info)) {
      requestedColumnsLeft.emplace_back(info);
      success=true;
    } else if (right->require(info)) {
      success=true;
      requestedColumnsRight.emplace_back(info);
    }
    if (!success)
      return false;

    tmpResults.emplace_back();
    requestedColumns.emplace(info);
  }
  return true;
}
//---------------------------------------------------------------------------
void ParallelHashJoin::run()
  // Run
{
  left->require(pInfo.left);
  right->require(pInfo.right);

  // Asynchronusly execute Scan Operations
  if(dynamic_cast<Scan*>(left.get())){
    auto left_job = pool_for_interoperation.EnqueueJob([this] { left->run(); });
    if (dynamic_cast<Scan *>(right.get())) {
      auto right_job =
          pool_for_interoperation.EnqueueJob([this] { right->run(); });
      left_job.wait();
      right_job.wait();
    } else {
      right->run();
      left_job.wait();
    }
  }else{
    if (dynamic_cast<Scan *>(right.get())) {
      auto right_job =
          pool_for_interoperation.EnqueueJob([this] { right->run(); });
      left->run();
      right_job.wait();
    } else {
      left->run();
      right->run();
    }
  }

  // Use smaller input for build
  if (left->resultSize>right->resultSize) {
    swap(left,right);
    swap(pInfo.left,pInfo.right);
    swap(requestedColumnsLeft,requestedColumnsRight);
  }

  auto leftInputData=left->getResults();
  auto rightInputData=right->getResults();

  // Resolve the input columns
  unsigned resColId=0;
  for (auto& info : requestedColumnsLeft) {
    copyLeftData.push_back(leftInputData[left->resolve(info)]);
    select2ResultColId[info]=resColId++;
  }
  for (auto& info : requestedColumnsRight) {
    copyRightData.push_back(rightInputData[right->resolve(info)]);
    select2ResultColId[info]=resColId++;
  }

  auto leftColId=left->resolve(pInfo.left);
  auto rightColId=right->resolve(pInfo.right);

  auto leftKeyColumn=leftInputData[leftColId];
  auto rightKeyColumn=rightInputData[rightColId];

  constexpr size_t JoinPairSize = sizeof(uint64_t) * 2;
  constexpr size_t BlockSize = L2CacheLineSize / JoinPairSize;

  using JoinPairs = std::vector<std::pair<uint64_t, uint64_t>>;

  // 1. Partition Phase
  std::vector<std::vector<size_t>> left_prefix_sums(PartitionSize);
  std::vector<std::vector<size_t>> right_prefix_sums(PartitionSize);
  std::vector<size_t> left_partial_sum(PartitionSize + 1), right_partial_sum(PartitionSize + 1);
  JoinPairs left_partition(left->resultSize), right_partition(right->resultSize);
  {
    // 1-1. Compute offsets
    const auto left_partition_job_size = (left->resultSize + BlockSize - 1) / BlockSize;
    const auto right_partition_job_size = (right->resultSize + BlockSize - 1) / BlockSize;
    std::vector<std::future<std::vector<uint64_t>>> compute_offset_jobs;
    compute_offset_jobs.reserve(left_partition_job_size + right_partition_job_size);

    // Left Partitioning
    for(auto& prefix_sum : left_prefix_sums){
      prefix_sum.reserve(left_partition_job_size + 1);
      prefix_sum.emplace_back(0);
    }
    
    // Distribute jobs per cache line
    for (size_t i = 0; i < left_partition_job_size; ++i){
      size_t s = i * BlockSize, e = std::min<size_t>((i+1) * BlockSize, left->resultSize);
      compute_offset_jobs.emplace_back(pool_for_interoperation.EnqueueJob([s, e, leftKeyColumn]{
        std::vector<uint64_t> histogram(PartitionSize);
        for(size_t i = s; i < e; ++i){
          ++histogram[leftKeyColumn[i] % PartitionSize];
        }
        return histogram;
      }));
    }

    // Right Partitioning
    for(auto& prefix_sum : right_prefix_sums){
      prefix_sum.reserve(right_partition_job_size + 1);
      prefix_sum.emplace_back(0);
    }

    // Distribute jobs per cache line
    for (size_t i = 0; i < right_partition_job_size; ++i){
      size_t s = i * BlockSize, e = std::min<size_t>((i+1) * BlockSize, right->resultSize);
      compute_offset_jobs.emplace_back(pool_for_interoperation.EnqueueJob([s, e, rightKeyColumn]{
        std::vector<uint64_t> histogram(PartitionSize);
        for(size_t i = s; i < e; ++i){
          ++histogram[rightKeyColumn[i] % PartitionSize];
        }
        return histogram;
      }));
    }

    // Wait for partitioned histograms
    
    // Merge left histograms
    for(size_t i = 0; i < left_partition_job_size; ++i){
      auto histogram = compute_offset_jobs[i].get();
      for(size_t j = 0; j < PartitionSize; ++j){
        left_prefix_sums[j].emplace_back(left_prefix_sums[j].back() + histogram[j]);
      }
    }

    // Calculate left partial sum
    for(size_t j = 0; j < PartitionSize; ++j){
      left_partial_sum[j + 1] = left_partial_sum[j] + left_prefix_sums[j].back();  
    }

    // Merge right histograms
    for(size_t i = 0; i < right_partition_job_size; ++i){
      auto histogram = compute_offset_jobs[i + left_partition_job_size].get();
      for(size_t j = 0; j < PartitionSize; ++j){
        right_prefix_sums[j].emplace_back(right_prefix_sums[j].back() + histogram[j]);
      }
    }

    // Calculate right partial sum
    for(size_t j = 0; j < PartitionSize; ++j){
      right_partial_sum[j + 1] = right_partial_sum[j] + right_prefix_sums[j].back();
    }

    // 1-2. Construct partitions
    std::vector<std::future<void>> partition_jobs;
    partition_jobs.reserve(left_partition_job_size + right_partition_job_size);

    for(size_t i = 0; i < left_partition_job_size; ++i){
      size_t s = i * BlockSize, e = std::min<size_t>((i+1) * BlockSize, left->resultSize);
      partition_jobs.emplace_back(pool_for_interoperation.EnqueueJob([this, job_idx = i, s, e, leftKeyColumn, &left_partition, &left_partial_sum, &left_prefix_sums]() {
        // Fill the left partition
        for(size_t i = s; i < e; ++i){
          auto key = leftKeyColumn[i];
          auto partition_id = key % PartitionSize;
          const size_t start_offset = left_partial_sum[partition_id];
          assert(start_offset + left_prefix_sums[partition_id][job_idx] < left->resultSize);
          left_partition[start_offset + left_prefix_sums[partition_id][job_idx]++] = {key, i};
        }
      }));
    }

    for(size_t i = 0; i < right_partition_job_size; ++i){
      size_t s = i * BlockSize, e = std::min<size_t>((i+1) * BlockSize, right->resultSize);
      partition_jobs.emplace_back(pool_for_interoperation.EnqueueJob([this, job_idx = i, s, e, rightKeyColumn, &right_partition, &right_partial_sum, &right_prefix_sums]() {
        // Fill the right partition
        for(size_t i = s; i < e; ++i){
          auto key = rightKeyColumn[i];
          auto partition_id = key % PartitionSize;
          const size_t start_offset = right_partial_sum[partition_id];
          assert(start_offset + right_prefix_sums[partition_id][job_idx] < right->resultSize);
          right_partition[start_offset + right_prefix_sums[partition_id][job_idx]++] = {key, i};
        }
      }));
    }

    for(auto&& job : partition_jobs){
      job.wait();
    }
  }

  // Build and Probe Phase
  std::vector<size_t> join_start_idx(PartitionSize + 1);
  {
    std::vector<std::future<size_t>> build_probe_jobs;
    build_probe_jobs.reserve(PartitionSize);

    for(size_t i = 0; i < PartitionSize; ++i){
      // Build Phase
      build_probe_jobs.emplace_back(pool_for_interoperation.EnqueueJob(
          [this, partition_id = i, &left_partition, &right_partition,
           &left_partial_sum, &right_partial_sum] {
            auto s = left_partial_sum[partition_id];
            auto e = left_partial_sum[partition_id + 1];

            // Build a hashtable of left partitions
            hashTables_left[partition_id].reserve(2 * (e - s));
            for (size_t i = s; i < e; ++i) {
              hashTables_left[partition_id][left_partition[i].first / PartitionSize].emplace_back(left_partition[i].second);
            }

            // Probe Phase
            s = right_partial_sum[partition_id];
            e = right_partial_sum[partition_id + 1];

            // Count the number of join results of this partition
            size_t result_size = 0;
            for (size_t i = s; i < e; ++i) {
              auto key = right_partition[i].first;
              // if(auto it = hashTables_left[partition_id].find(key / PartitionSize); it != hashTables_left[partition_id].end()){
              if(hashTables_left[partition_id].count(key / PartitionSize)){
                // result_size += it->second.size();
                result_size += hashTables_left[partition_id][key / PartitionSize].size();
              }
            }

            return result_size;
          }));
    }

    // Make a partial index sum for each joined partition
    size_t idx = 0;
    for(auto&& job : build_probe_jobs){
      auto num_tuples_in_partition = job.get();
      join_start_idx[idx+1] = join_start_idx[idx] + num_tuples_in_partition;
      ++idx;
    }

    resultSize = join_start_idx[PartitionSize];
  }

  // Add data phase
  {
    std::vector<std::future<void>> add_data_jobs;
    add_data_jobs.reserve(PartitionSize);

    // Preoccupy the memory
    for(auto& column : tmpResults)
      column.reserve(resultSize);

    // Add data
    for(size_t i = 0; i < PartitionSize; ++i){
      add_data_jobs.emplace_back(pool_for_intraoperation.EnqueueJob(
          [this, partition_id = i, start_idx = join_start_idx[i],
           &right_partition, &right_partial_sum]() mutable {
            
            auto s = right_partial_sum[partition_id];
            auto e = right_partial_sum[partition_id + 1];

            const size_t num_left_cols = copyLeftData.size();
            const size_t num_right_cols = copyRightData.size();
            size_t row_idx = start_idx;

            for (size_t i = s; i < e; ++i) {
              auto key = right_partition[i].first;
              auto right_idx = right_partition[i].second;
              auto range = hashTables_left[partition_id].find(key / PartitionSize);
              if(range != hashTables_left[partition_id].end()){
                for (auto left_idx : range->second) {
                  // left_idx, right_idx 페어추가
                  for (size_t j = 0; j < num_left_cols; ++j) {
                    tmpResults[j][row_idx] = copyLeftData[j][left_idx];
                  }
                  for (size_t j = 0; j < num_right_cols; ++j) {
                    tmpResults[j + num_left_cols][row_idx] = copyRightData[j][right_idx];
                  }
                  ++row_idx;
                }
              }
            }
          }));
    }

    for (auto&& job : add_data_jobs)
      job.wait();
  }
}
//---------------------------------------------------------------------------
bool ParallelSortMergeJoin::require(SelectInfo info){
  if (requestedColumns.count(info)==0) {
    bool success=false;
    if(left->require(info)) {
      requestedColumnsLeft.emplace_back(info);
      success=true;
    } else if (right->require(info)) {
      success=true;
      requestedColumnsRight.emplace_back(info);
    }
    if (!success)
      return false;

    tmpResults.emplace_back();
    requestedColumns.emplace(info);
  }
  return true;
}
//---------------------------------------------------------------------------
void ParallelSortMergeJoin::copy2Result(uint64_t leftId,uint64_t rightId){
  unsigned relColId=0;
  for (unsigned cId=0;cId<copyLeftData.size();++cId)
    tmpResults[relColId++].push_back(copyLeftData[cId][leftId]);

  for (unsigned cId=0;cId<copyRightData.size();++cId)
    tmpResults[relColId++].push_back(copyRightData[cId][rightId]);
  ++resultSize;
}
//---------------------------------------------------------------------------
void ParallelSortMergeJoin::run(){
  left->require(pInfo.left);
  right->require(pInfo.right);

  // Asynchronusly execute Scan Operations
  if(dynamic_cast<Scan*>(left.get())){
    auto left_job = pool_for_interoperation.EnqueueJob([this] { left->run(); });
    if (dynamic_cast<Scan *>(right.get())) {
      auto right_job =
          pool_for_interoperation.EnqueueJob([this] { right->run(); });
      left_job.wait();
      right_job.wait();
    } else {
      right->run();
      left_job.wait();
    }
  }else{
    if (dynamic_cast<Scan *>(right.get())) {
      auto right_job =
          pool_for_interoperation.EnqueueJob([this] { right->run(); });
      left->run();
      right_job.wait();
    } else {
      left->run();
      right->run();
    }
  }

  // Use smaller input for build
  if (left->resultSize>right->resultSize) {
    swap(left,right);
    swap(pInfo.left,pInfo.right);
    swap(requestedColumnsLeft,requestedColumnsRight);
  }

  auto leftInputData=left->getResults();
  auto rightInputData=right->getResults();

  // Resolve the input columns
  unsigned resColId=0;
  for (auto& info : requestedColumnsLeft) {
    copyLeftData.push_back(leftInputData[left->resolve(info)]);
    select2ResultColId[info]=resColId++;
  }
  for (auto& info : requestedColumnsRight) {
    copyRightData.push_back(rightInputData[right->resolve(info)]);
    select2ResultColId[info]=resColId++;
  }

  auto leftColId=left->resolve(pInfo.left);
  auto rightColId=right->resolve(pInfo.right);

  using KeyIdxPair = std::pair<uint64_t, size_t>;
  std::vector<KeyIdxPair> leftColKeyIdxPairs;
  leftColKeyIdxPairs.reserve(left->resultSize);
  for(size_t i=0;i<left->resultSize;++i)
    leftColKeyIdxPairs.emplace_back(leftInputData[leftColId][i],i);
  std::sort(leftColKeyIdxPairs.begin(), leftColKeyIdxPairs.end());

  std::vector<KeyIdxPair> rightColKeyIdxPairs;
  rightColKeyIdxPairs.reserve(right->resultSize);
  for(size_t i=0;i<right->resultSize;++i)
    rightColKeyIdxPairs.emplace_back(rightInputData[rightColId][i],i);
  std::sort(rightColKeyIdxPairs.begin(), rightColKeyIdxPairs.end());

  auto cmp = [](const KeyIdxPair& a, const KeyIdxPair& b){ return a.first < b.first; };
  
  auto leftColKeyIdxPairsIter = leftColKeyIdxPairs.begin();
  while(leftColKeyIdxPairsIter != leftColKeyIdxPairs.end()){
    const auto joinKey = leftColKeyIdxPairsIter->first;
    auto [left_s, left_e] = std::equal_range(leftColKeyIdxPairs.begin(), leftColKeyIdxPairs.end(), KeyIdxPair{joinKey, 0}, cmp);
    auto [right_s, right_e] = std::equal_range(rightColKeyIdxPairs.begin(), rightColKeyIdxPairs.end(), KeyIdxPair{joinKey, 0}, cmp);

    while(left_s != left_e){
      while(right_s != right_e){
        copy2Result(left_s->second, right_s->second);
        ++right_s;
      }
      ++left_s;
    }

    leftColKeyIdxPairsIter = left_e;
  }
}
//---------------------------------------------------------------------------
void ParallelChecksum::run()
  // Run
{
  for (auto& sInfo : colInfo) {
    input->require(sInfo);
  }
  input->run();
  
  auto results=input->getResults();
  checkSums.reserve(colInfo.size());

  const size_t datasize_per_column = input->resultSize * sizeof(uint64_t);
  const size_t total_size = datasize_per_column * colInfo.size();

  if(total_size <= L2CacheLineSize){
    // just calculate checksums sequentially
    for (auto& sInfo : colInfo) {
      auto colId=input->resolve(sInfo);
      auto& data=results[colId];
      uint64_t checksum = std::accumulate(data, data+input->resultSize, uint64_t(0));
      checkSums.push_back(checksum);
    }
  }else{
    // calculate checksums in parallel
    const auto chunk_size = 2 * L2CacheLineSize / sizeof(uint64_t);
    const auto job_size = (input->resultSize + chunk_size - 1) / chunk_size;

    std::vector<std::future<uint64_t>> tasks;
    tasks.reserve(colInfo.size() * job_size);

    for (auto& sInfo : colInfo) {
      auto colId=input->resolve(sInfo);
      auto data=results[colId];

      for(size_t i = 0; i < job_size; ++i){
        auto s = data + i * chunk_size, e = std::min(data + (i + 1) * chunk_size, data + input->resultSize);
        tasks.emplace_back(pool_for_intraoperation.EnqueueJob([s, e, this]() -> uint64_t {
          return std::accumulate(s, e, uint64_t(0));
        }));
      }
    }

    // Wait for all checksums to be calculated
    uint64_t checksum;
    size_t column_cnt = 0;
    for (auto& sInfo : colInfo) {
      checksum = 0;
      for(size_t i = 0; i < job_size; ++i){
        checksum += tasks[column_cnt * job_size + i].get();
      }
      checkSums.push_back(checksum); // store checksum
      ++column_cnt;
    }
  }

  // Store the final result size
  resultSize=input->resultSize;
}
//---------------------------------------------------------------------------
void ParallelSelfJoin::copy2Result(uint64_t id)
  // Copy to result
{
  for (unsigned cId=0;cId<copyData.size();++cId)
    tmpResults[cId].push_back(copyData[cId][id]);
  ++resultSize;
}
//---------------------------------------------------------------------------
bool ParallelSelfJoin::require(SelectInfo info)
  // Require a column and add it to results
{
  if (requiredIUs.count(info))
    return true;
  if(input->require(info)) {
    tmpResults.emplace_back();
    requiredIUs.emplace(info);
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
void ParallelSelfJoin::run()
  // Run
{
  input->require(pInfo.left);
  input->require(pInfo.right);
  input->run();
  inputData=input->getResults();

  for (auto& iu : requiredIUs) {
    auto id=input->resolve(iu);
    copyData.emplace_back(inputData[id]);
    select2ResultColId.emplace(iu,copyData.size()-1);
  }

  auto leftColId=input->resolve(pInfo.left);
  auto rightColId=input->resolve(pInfo.right);

  auto leftCol=inputData[leftColId];
  auto rightCol=inputData[rightColId];

  if(copyData.size() * input->resultSize < 10000){
    for (uint64_t i=0;i<input->resultSize;++i) {
      if (leftCol[i]==rightCol[i])
        copy2Result(i);
    }
  }else{
    using JoinRecords = std::vector<std::vector<uint64_t>>;
    using Records_Size_Pair = pair<JoinRecords, size_t>;
    std::vector<std::future<Records_Size_Pair>> tasks;
    const auto job_count = pool_for_intraoperation.GetNumThreads();
    const auto chunk_size = (input->resultSize + job_count - 1) / job_count;

    tasks.reserve(job_count);
    for(size_t i = 0; i < job_count; ++i){
      const auto begin = i * chunk_size,
                 end = min((i + 1) * chunk_size, input->resultSize);
      tasks.emplace_back(pool_for_intraoperation.EnqueueJob(
          [leftCol, rightCol, &copyData = this->copyData, &_tmpResults = this->tmpResults](size_t begin, size_t end) {
            JoinRecords tmpResults = _tmpResults;

            for (auto i = begin; i != end; ++i){
              if (leftCol[i] == rightCol[i]){
                for (unsigned cId=0;cId<copyData.size();++cId)
                  tmpResults[cId].push_back(copyData[cId][i]);
              }
            }

            return Records_Size_Pair{std::move(tmpResults), tmpResults[0].size()};
          }, begin, end));
    }

    std::vector<Records_Size_Pair> results;
    results.reserve(job_count);

    for (auto &task : tasks) {
      task.wait();
      results.emplace_back(task.get());
    }

    for (auto& result : results){
      resultSize += result.second;
    }

    size_t col_idx = 0;
    for(auto& column : tmpResults){
      column.reserve(resultSize);
      for(auto& result : results){
        // column.insert(column.end(), std::make_move_iterator(result.first[col_idx].begin()), std::make_move_iterator(result.first[col_idx].end()));
        column.insert(column.end(), result.first[col_idx].begin(), result.first[col_idx].end());
      }
      ++col_idx;
    }
  }
}
//---------------------------------------------------------------------------