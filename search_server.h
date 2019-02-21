#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <future>

#include "synchronize.h"

using namespace std;

struct IndexItem
{
  size_t docid;
  size_t hitcount;
};

class InvertedIndex {
public:
  InvertedIndex() = default;
  InvertedIndex(InvertedIndex&& other);
  void operator=(InvertedIndex&& other);
  
  void Add(const string& document);
  vector<IndexItem> Lookup(const string& word) const;

private:
  map<string, vector<IndexItem>> index;
  vector<IndexItem> empty;
  size_t last_docid = 0;
  const size_t MAX_WORDS = 1000;
};

class SearchServer {
public:
  SearchServer() = default;
  explicit SearchServer(istream& document_input);
  void UpdateDocumentBase(istream& document_input);
  void AddQueriesStream(istream& query_input, ostream& search_results_output);
  void WaitAll();

private:
  const size_t MAX_DOC_ID = 50'000;
  const size_t MAX_QUERY_WORDS = 10;
  const size_t MAX_RESULTS = 5;
  const size_t MAX_THREADS = 6;

  Synchronized<InvertedIndex> index;
  vector<future<void>> futures;

  void AddQueriesStreamSingleThread(istream& query_input, ostream& search_results_output);
  
};
