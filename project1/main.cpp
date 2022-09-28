#include <iostream>
#include <queue>
#include "Joiner.hpp"
#include "Parser.hpp"
#include "ThreadPool.hpp"

static ThreadPool query_processing_job_pool(ThreadPool::GetSupportedHardwareThreads()/2);

using namespace std;
//---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
   Joiner joiner;
   // Read join relations
   std::vector<std::future<Relation>> read_tasks;

   string line;
   while (getline(cin, line)) {
      if (line == "Done") break;
      read_tasks.emplace_back(query_processing_job_pool.EnqueueJob([line = std::move(line)]{
         return Relation(line.c_str());
      }));
   }

   // Read all relations
   for(auto&& task : read_tasks) {
      joiner.addRelation(task.get());
   }

   // Preparation phase (not timed)
   // Build histograms, indexes,...
   //
   std::queue<std::future<std::string>> results;

   while (getline(cin, line)) {
      if (line == "F") continue; // End of a batch
      results.push(query_processing_job_pool.EnqueueJob([&joiner]
         (std::string query) {
         QueryInfo i;
         i.parseQuery(query);
         return joiner.join(i);
      }, std::move(line)));

      while(!results.empty() && results.front().valid()){
         std::cout << results.front().get();
         results.pop();
      }
   }
   while(!results.empty()){
      std::cout << results.front().get();
      results.pop();
   }
   return 0;
}
