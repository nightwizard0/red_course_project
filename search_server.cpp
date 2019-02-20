#include "search_server.h"
#include "iterator_range.h"
#include "stat_profile.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <string_view>

vector<string_view> SplitIntoWords(const string& s, size_t capacity) 
{
  string_view str = s;
  vector<string_view> result; result.reserve(capacity);

  while (true) 
  {
    size_t space = str.find(' ');
    
    auto word = str.substr(0, space); 
    
    if (!word.empty())
      result.push_back(word);

    if (space == str.npos) 
    {
      break;
    } 
    else
    {
      str.remove_prefix(space + 1);
    }
  }
  
  return result;
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
  vector<pair<size_t, size_t*>> search_results; search_results.reserve(MAX_DOC_ID);

  for (string current_query; getline(query_input, current_query); ) 
  {
    vector<string_view> words;
    
    {
      STAT_DURATION(profiler, "SplitIntoWords");
      words = SplitIntoWords(current_query, MAX_QUERY_WORDS);
    }

    fill(docid_count.begin(), docid_count.end(), 0);
    search_results.clear();

    for (const auto& word : words) 
    {
      STAT_DURATION(profiler, "Lookup");

      for (const IndexItem& item : index.Lookup(string(word)))
      {
        if (docid_count[item.docid] == 0) 
        {
          search_results.emplace_back(make_pair(item.docid, &docid_count[item.docid]));
        }

        docid_count[item.docid] += item.hitcount;
      }
    }

    {
      STAT_DURATION(profiler, "Sort");

      auto middle = search_results.size() > MAX_RESULTS ? begin(search_results) + MAX_RESULTS 
                                                        : end(search_results);
    
      partial_sort(
        begin(search_results), middle,
        end(search_results),
        [](pair<size_t, size_t*> lhs, pair<size_t, size_t*> rhs) {
          int64_t lhs_docid = lhs.first;
          auto lhs_hit_count = *lhs.second;
          int64_t rhs_docid = rhs.first;
          auto rhs_hit_count = *rhs.second;
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
        << "hitcount: " << *hitcount << '}';
    }

    search_results_output << endl;
  }
}

InvertedIndex::InvertedIndex(InvertedIndex&& other)
  : index(move(other.index)),
    last_docid(other.last_docid)
{ 
  other.last_docid = 0;
}

void InvertedIndex::operator=(InvertedIndex&& other)
{
  index = move(other.index);
  last_docid = other.last_docid;
  other.last_docid = 0;
}

void InvertedIndex::Add(const string& document) {

  const size_t docid =  last_docid++;
  map<string_view, size_t*> words;
  for (auto& word : SplitIntoWords(document, MAX_WORDS)) {
    auto it = words.find(word);
    if (it == words.end())
    {
      auto& point = index[string(word)];
      point.push_back({docid, 1});
      words[word] = &point[point.size() - 1].hitcount;
    }
    else
    {
      *(it->second) = *(it->second) + 1;
    }
  }
}

const vector<IndexItem>& InvertedIndex::Lookup(const string& word) const {
  if (auto it = index.find(word); it != index.end()) {
    return it->second;
  } else {
    return empty;
  }
}
