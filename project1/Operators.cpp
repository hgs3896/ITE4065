#include <ConcurrentHashmap.hpp>
#include <Operators.hpp>
#include <ThreadPool.hpp>
#include <cassert>
#include <list>
#include <future>
#include <iostream>
#include <vector>

static ThreadPool pool;

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
bool ParallelJoin::require(SelectInfo info)
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
void ParallelJoin::run()
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

  const auto job_count = pool.GetNumThreads();

  using JoinRecords = std::vector<std::vector<uint64_t>>;
  // std::vector<HT> hashTables(job_count);
  // concurrent_hashmap<uint64_t, uint64_t> concurrentHashTable(job_count, left->resultSize);
  // {
  //   auto leftKeyColumn=leftInputData[leftColId];
  //   const auto chunk_size = (left->resultSize + job_count - 1) / job_count;
  //   std::vector<future<void>> population_tasks;
  //   for (size_t i = 0; i < job_count; ++i) {
  //     const auto start = i * chunk_size;
  //     const auto end = std::min(start + chunk_size, left->resultSize);
  //     if(start >= end) continue;

  //     population_tasks.emplace_back(pool.EnqueueJob([leftKeyColumn, &concurrentHashTable](size_t begin, size_t end){
  //       for (auto i=begin;i!=end;++i) {
  //         concurrentHashTable.insert(leftKeyColumn[i], i);
  //       }
  //     }, start, end));
  //   }

  //   for (auto& population_task : population_tasks) {
  //     population_task.wait();
  //   }

  //   population_tasks.clear();

  //   for (size_t i = 0; i < job_count; ++i) {
  //     population_tasks.emplace_back(
  //         pool.EnqueueJob([i, &hashTables, &concurrentList = concurrentHashTable.get_bucket(i)]{
  //         hashTables[i].reserve(concurrentList.size());
  //         for (auto iter = concurrentList.get_head(); iter; iter = iter->next) {
  //           hashTables[i].emplace(iter->data.first, iter->data.second);
  //         }
  //       })
  //     );
  //   }

  //   for (auto& population_task : population_tasks) {
  //     population_task.wait();
  //   }
  // }

  // Build phase (Sequential)
  auto leftKeyColumn=leftInputData[leftColId];
  hashTable.reserve(left->resultSize*2);
  for (uint64_t i=0,limit=i+left->resultSize;i!=limit;++i) {
    hashTable.emplace(leftKeyColumn[i],i);
  }

  vector<future<JoinRecords>> tasks;

  const auto chunk_size = (right->resultSize + job_count - 1) / job_count;
  tasks.reserve(job_count);

  // Probe phase
  auto rightKeyColumn=rightInputData[rightColId];
  for (size_t job_id = 0; job_id < job_count; ++job_id) {
    uint64_t l = job_id * chunk_size, r = min((job_id+1) * chunk_size, right->resultSize);
    if(l >= r) continue;

    tasks.emplace_back(
        pool.EnqueueJob([rightKeyColumn,
                         &copyLeftData = this->copyLeftData,
                         &copyRightData = this->copyRightData,
                         &_tmpResults = this->tmpResults,
                         &hashTable = this->hashTable]
                        //  &hashTables,
                        //  &concurrentHashTable]
                        (uint64_t l, uint64_t r) {
          auto tmpResults = JoinRecords(_tmpResults.size());

          for (uint64_t i = l; i != r; ++i) {
            auto rightKey = rightKeyColumn[i];
            auto range = hashTable.equal_range(rightKey);
            // auto range = hashTables[concurrentHashTable.hash(rightKey) % concurrentHashTable.get_bucket_size()].equal_range(rightKey);

            for (auto iter = range.first; iter != range.second; ++iter) {
              const auto &leftId = iter->second;
              const auto &rightId = i;

              unsigned relColId = 0;
              for (unsigned cId = 0; cId < copyLeftData.size(); ++cId)
                tmpResults[relColId++].emplace_back(copyLeftData[cId][leftId]);

              for (unsigned cId = 0; cId < copyRightData.size(); ++cId)
                tmpResults[relColId++].emplace_back(copyRightData[cId][rightId]);
            }
          }

          return tmpResults;
        }, l, r));
  }

  for (auto& task : tasks) {
    task.wait();
    auto&& temp = task.get();
    
    int column_idx = 0;
    for(auto& column : tmpResults){
      column.insert(column.end(), std::make_move_iterator(temp[column_idx].begin()), std::make_move_iterator(temp[column_idx].end()));
      ++column_idx;
    }
  }
  resultSize = tmpResults.size() ? tmpResults[0].size() : 0;
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

  if(colInfo.size() * input->resultSize < 10000){
    for (auto& sInfo : colInfo) {
      auto colId=input->resolve(sInfo);
      auto resultCol=results[colId];
      uint64_t sum=0;
      resultSize=input->resultSize;
      for (auto iter=resultCol,limit=iter+input->resultSize;iter!=limit;++iter)
        sum+=*iter;
      checkSums.push_back(sum);
    }
  }else{
    std::vector<std::future<uint64_t>> tasks;
    tasks.resize(colInfo.size());
    
    size_t col_idx;

    for (col_idx = 0;col_idx < colInfo.size(); ++col_idx) {
      auto& sInfo = colInfo[col_idx];
      auto colId=input->resolve(sInfo);
      auto resultCol=results[colId];

      tasks[col_idx] = pool.EnqueueJob(
          [](uint64_t *begin, uint64_t *end) {
            uint64_t sum = 0;
            for (auto iter = begin; iter != end; ++iter)
              sum += *iter;
            return sum;
          },
          resultCol, &resultCol[input->resultSize]);
    }

    resultSize=input->resultSize;
    for (col_idx = 0; col_idx < colInfo.size(); ++col_idx) {
      uint64_t sum = 0;
      
      tasks[col_idx].wait();
      sum += tasks[col_idx].get();
      checkSums.push_back(sum);
    }
  }
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
    const auto job_count = pool.GetNumThreads();
    const auto chunk_size = (input->resultSize + job_count - 1) / job_count;

    tasks.reserve(job_count);
    for(size_t i = 0; i < job_count; ++i){
      const auto begin = i * chunk_size,
                 end = min((i + 1) * chunk_size, input->resultSize);
      tasks.emplace_back(pool.EnqueueJob(
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
        column.insert(column.end(), std::make_move_iterator(result.first[col_idx].begin()), std::make_move_iterator(result.first[col_idx].end()));
      }
      ++col_idx;
    }
  }
}
//---------------------------------------------------------------------------