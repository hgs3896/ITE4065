#pragma once
#include <vector>
#include <cstdint>
#include <set>
#include "Operators.hpp"
#include "Relation.hpp"
#include "Parser.hpp"
//---------------------------------------------------------------------------
class Joiner {

  /// Add scan to query
  std::unique_ptr<Operator> addScan(std::set<unsigned>& usedRelations,SelectInfo& info,QueryInfo& query) const;

  public:
  /// The relations that might be joined
  std::vector<Relation> relations;
  /// Add relation
  void addRelation(const char* fileName);
  void addRelation(Relation&& relation);
  /// Get relation
  const Relation& getRelation(unsigned id) const;
  /// Joins a given set of relations
  std::string join(QueryInfo& i) const;
};
//---------------------------------------------------------------------------
