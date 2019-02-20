#include "search_server.h"
#include "iterator_range.h"
#include "stat_profile.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

vector<string> SplitIntoWords(const string& line) {
  istringstream words_input(line);
  return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream& document_input) {
  UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {
  InvertedIndex new_index;

  for (string current_document; getline(document_input, current_document); ) {
    new_index.Add(move(current_document));
  }

  index = move(new_index);
}

void SearchServer::AddQueriesStream(istream& query_input, ostream& search_results_output) 
{
  Profiler profiler;

  vector<size_t> docid_count; docid_count.resize(MAX_DOC_ID, 0);

  for (string current_query; getline(query_input, current_query); ) 
  {
    vector<string> words;
    
    {
      STAT_DURATION(profiler, "SplitIntoWords");
      words = SplitIntoWords(current_query);
    }

    fill(docid_count.begin(), docid_count.end(), 0);
    size_t total = 0;

    for (const auto& word : words) 
    {
      STAT_DURATION(profiler, "Lookup");

      for (const size_t docid : index.Lookup(word)) 
      {
        if (docid_count[docid] == 0) total++;
        docid_count[docid]++;
      }
    }

    vector<pair<size_t, size_t>> search_results; search_results.reserve(total);

    for (size_t i = 0; i < docid_count.size() && total > 0; i++)
    {
      STAT_DURATION(profiler, "Iterate vector");

      if (docid_count[i] != 0)
      {
        search_results.push_back({i, docid_count[i]});
        total--;
      }
    }

    
    {
      STAT_DURATION(profiler, "Sort");

      auto middle = search_results.size() > MAX_RESULTS ? begin(search_results) + MAX_RESULTS 
                                                        : end(search_results);
    
      partial_sort(
        begin(search_results), middle,
        end(search_results),
        [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
          int64_t lhs_docid = lhs.first;
          auto lhs_hit_count = lhs.second;
          int64_t rhs_docid = rhs.first;
          auto rhs_hit_count = rhs.second;
          return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid);
        }
      );
    }

    search_results_output << current_query << ':';
    for (auto [docid, hitcount] : Head(search_results, 5)) 
    {
      STAT_DURATION(profiler, "MakeOutput");
    
      search_results_output << " {"
        << "docid: " << docid << ", "
        << "hitcount: " << hitcount << '}';
    }

    search_results_output << endl;
  }
}

void InvertedIndex::Add(const string& document) {

  const size_t docid =  last_docid++;
  for (const auto& word : SplitIntoWords(document)) {
    index[word].push_back(docid);
  }
}

list<size_t> InvertedIndex::Lookup(const string& word) const {
  if (auto it = index.find(word); it != index.end()) {
    return it->second;
  } else {
    return {};
  }
}
