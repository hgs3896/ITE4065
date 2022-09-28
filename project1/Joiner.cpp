#include "Joiner.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <set>
#include <sstream>
#include <vector>
#include "Parser.hpp"
#include "ThreadPool.hpp"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
void Joiner::addRelation(const char* fileName)
// Loads a relation from disk
{
  relations.emplace_back(fileName);
}
void Joiner::addRelation(Relation&& relation)
// Loads a relation from disk
{
  relations.emplace_back(std::move(relation));
}
//---------------------------------------------------------------------------
const Relation& Joiner::getRelation(unsigned relationId) const
// Loads a relation from disk
{
  if (relationId >= relations.size()) {
    cerr << "Relation with id: " << relationId << " does not exist" << endl;
    throw;
  }
  return relations[relationId];
}
//---------------------------------------------------------------------------
unique_ptr<Operator> Joiner::addScan(set<unsigned>& usedRelations,SelectInfo& info,QueryInfo& query) const
  // Add scan to query
{
  usedRelations.emplace(info.binding);
  vector<FilterInfo> filters;
  for (auto& f : query.filters) {
    if (f.filterColumn.binding==info.binding) {
      filters.emplace_back(f);
    }
  }
  
  const auto& relation = getRelation(info.relId);
  if(filters.empty()){
    return make_unique<Scan>(relation,info.binding); 
  }else{
    return relation.size * relation.columns.size() * sizeof(uint64_t) > L2CacheLineSize ?
      make_unique<ParallelFilterScan>(relation,filters) :
      make_unique<FilterScan>(relation,filters);
  }
}
//---------------------------------------------------------------------------
enum QueryGraphProvides {  Left, Right, Both, None };
//---------------------------------------------------------------------------
static QueryGraphProvides analyzeInputOfJoin(set<unsigned>& usedRelations,SelectInfo& leftInfo,SelectInfo& rightInfo)
  // Analyzes inputs of join
{
  bool usedLeft=usedRelations.count(leftInfo.binding);
  bool usedRight=usedRelations.count(rightInfo.binding);

  if (usedLeft^usedRight)
    return usedLeft?QueryGraphProvides::Left:QueryGraphProvides::Right;
  if (usedLeft&&usedRight)
    return QueryGraphProvides::Both;
  return QueryGraphProvides::None;
}
//---------------------------------------------------------------------------
string Joiner::join(QueryInfo& query) const
  // Executes a join query
{
  //cerr << query.dumpText() << endl;
  set<unsigned> usedRelations;

  // Query Optimization
  {
    extern ThreadPool pool_for_interoperation;

    auto applyFilter = []
      (const uint64_t* compareCol, size_t i, const FilterInfo& f){
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
    };

    auto getFilteredCounts = [this, &applyFilter]
      (size_t relationBinding, const std::vector<FilterInfo>& filters) -> size_t {
        size_t cnt = 0;
        
        const Relation& relation = getRelation(relationBinding);

        if(filters.empty())
          return relation.size;

        for(size_t i = 0; i < relation.size;++i){
          bool pass=true;
          for (auto& f : filters) {
            auto compareCol=relation.columns[f.filterColumn.colId];
            pass&=applyFilter(compareCol, i, f);
          }
          cnt += pass;
        }

        return cnt;
    };
    
    std::vector<std::pair<size_t, PredicateInfo>> filteredCounts;
    std::vector<std::future<size_t>> filteredCounts_jobs;
    filteredCounts.reserve(query.predicates.size());
    filteredCounts_jobs.reserve(query.predicates.size());
    
    for(const auto& pInfo : query.predicates){
      filteredCounts_jobs.emplace_back(pool_for_interoperation.EnqueueJob([this, &getFilteredCounts, &pInfo, &query] () -> size_t {
        std::vector<FilterInfo> leftFilters, rightFilters;
        const auto& leftRelation = getRelation(pInfo.left.relId);
        for(const auto& f : query.filters){
          if(f.filterColumn.relId == pInfo.left.relId && f.filterColumn.colId < leftRelation.columns.size()){
            leftFilters.emplace_back(f);
          }
        }
        size_t num_left_rows = getFilteredCounts(pInfo.left.relId, leftFilters);

        const auto& rightRelation = getRelation(pInfo.right.relId);
        for(const auto& f : query.filters){
          if(f.filterColumn.relId == pInfo.right.relId && f.filterColumn.colId < rightRelation.columns.size()){
            rightFilters.emplace_back(f);
          }
        }
        size_t num_right_rows = getFilteredCounts(pInfo.right.relId, rightFilters);
        return num_left_rows * num_right_rows;
      }));
    }

    size_t i = 0;
    for(auto& job : filteredCounts_jobs){
      size_t sz = job.get();
      filteredCounts.emplace_back(sz, std::move(query.predicates[i++]));
    }

    std::sort(filteredCounts.begin(), filteredCounts.end(),
      [](const std::pair<size_t, PredicateInfo>& a, const std::pair<size_t, PredicateInfo>& b){
        return a.first < b.first;
      });
    
    i = 0;
    for(auto&& p : filteredCounts){
      query.predicates[i++] = std::move(p.second);
    }
  }

  // We always start with the first join predicate and append the other joins to it (--> left-deep join trees)
  // You might want to choose a smarter join ordering ...
  auto& firstJoin=query.predicates[0];
  auto left=addScan(usedRelations,firstJoin.left,query);
  auto right=addScan(usedRelations,firstJoin.right,query);
  unique_ptr<Operator> root=make_unique<ParallelHashJoin>(move(left),move(right),firstJoin);

  for (unsigned i=1;i<query.predicates.size();++i) {
    auto& pInfo=query.predicates[i];
    auto& leftInfo=pInfo.left; auto& rightInfo=pInfo.right;
    unique_ptr<Operator> left, right;
    switch(analyzeInputOfJoin(usedRelations,leftInfo,rightInfo)) {
      case QueryGraphProvides::Left:
        left=move(root);
        right=addScan(usedRelations,rightInfo,query);
        root=make_unique<ParallelHashJoin>(move(left),move(right),pInfo);
        break;
      case QueryGraphProvides::Right:
        left=addScan(usedRelations,leftInfo,query);
        right=move(root);
        root=make_unique<ParallelHashJoin>(move(left),move(right),pInfo);
        break;
      case QueryGraphProvides::Both:
        // All relations of this join are already used somewhere else in the query.
        // Thus, we have either a cycle in our join graph or more than one join predicate per join.
        root=make_unique<SelfJoin>(move(root),pInfo);
        break;
      case QueryGraphProvides::None:
        // Process this predicate later when we can connect it to the other joins
        // We never have cross products
        query.predicates.push_back(pInfo);
        break;
    };
  }

  ParallelChecksum checkSum(move(root),query.selections);
  checkSum.run();

  stringstream out;
  auto& results=checkSum.checkSums;
  for (unsigned i=0;i<results.size();++i) {
    out << (checkSum.resultSize==0?"NULL":to_string(results[i]));
    if (i<results.size()-1)
      out << " ";
  }
  out << "\n";
  return out.str();
}
//---------------------------------------------------------------------------
