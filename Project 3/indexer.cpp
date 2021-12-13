/* Citations:
 * https://stackoverflow.com/questions/3450860/check-if-a-stdvector-contains-a-certain-object
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include "nlohmann/json.hpp"


// Avoiding use of `using namespace std;`
using std::unordered_map;
using std::map;
using std::vector;
using std::string;
using std::pair;
using std::cout;
using std::endl;


// Postings Class to store postings list
class Postings {
    unordered_map<int, vector<int>> postings_list;
    int count = 0;

    public:
        // Adds docId and pos pair to PostingsList
        void add_instance(int docId, int pos) {
            if (!postings_list.count(docId)) { postings_list.insert(std::make_pair(docId, vector<int>{})); }
            postings_list[docId].push_back(pos);
            count++;
        }

        // Returns pointer to the postings_list
        unordered_map<int, vector<int>>* get_postings_list() { return &postings_list; }

        // Returns the freq of the term
        int get_term_count() const { return count; }

        // Returns the number of documents this term occurs in
        int get_doc_count() { return (int) postings_list.size(); }
};

/* Global Variables */
// Maps inverted_list[term] = Postings
unordered_map<string, Postings*> inverted_list;

// Vector with docId as indices => doc_info[docId] = pair<sceneId, playId>
vector<pair<string, string>> doc_info;

// Maps play_count[playId] = count;
map<string, int> play_count;

// Maps scene_count[playId] = count;
map<string, int> scene_count;


/* API's for data access */
// Returns the frequency of the term
int get_term_freq(const string& term) { return inverted_list[term]->get_term_count();}

// Returns the number of docs this term occurs in
int get_doc_freq(const string& term) { return inverted_list[term]->get_doc_count(); }

// Returns the size of the vocab collection
int get_collection_size() { return (int) inverted_list.size(); }

// Returns the length of the document
int get_doc_length(int docId) { return scene_count[doc_info[docId].first]; }

// Returns the number of documents
int get_num_docs() { return (int) doc_info.size();}

// Returns the avg length of a play
double get_avg_scene_length() {
    double sum = 0;
    for (const auto& scene : scene_count) { sum += scene.second; }
    return sum / (double) scene_count.size();
}

// Returns the shortest play and length of that play
pair<string, int> get_shortest_scene() {
    return std::make_pair(scene_count.begin()->first, scene_count.begin()->second);
}

// Returns the longest play and length of that play
pair<string, int> get_longest_scene() {
    return std::make_pair(scene_count.end()->first, scene_count.end()->second);
}

// Returns the avg length of a scene
double get_avg_play_length() {
    double sum = 0;
    for (const auto& play : play_count) { sum += play.second; }
    return sum / (double) play_count.size();
}

// Returns the shortest play and length of that play
pair<string, int> get_shortest_play() {
    return std::make_pair(play_count.begin()->first, play_count.begin()->second);
}

// Returns the longest play and length of that play
pair<string, int> get_longest_play() {
    return std::make_pair(play_count.end()->first, play_count.end()->second);
}

// Returns the set of vocab
std::set<string> get_vocab() {
    std::set<string> vocab_collection;
    for (auto &item : inverted_list) { vocab_collection.insert(item.first); }
    return vocab_collection;
}

// Returns the playId given the docId
string get_playId(int docId) { return doc_info[docId].second; }

// Returns the sceneId given the docId
string get_sceneId(int docId) { return doc_info[docId].first; }


int main(int argc, char **argv) {
    if (argc < 3) {
        cout << "Usage: ./indexer <file.json> [-play] [-gt] [-phrase] queries" << endl;
        cout << "Default: Returns sceneId's and assumes all arguments are independent terms" << endl;
        cout << "To return a play, use the '-play' flag" << endl;
        cout << "To search for a phrase, use the '-phrase' flag" << endl;
        cout << "To do term frequency comparisons, use the '-gt' flag, and separate larger terms with '-gt'" << endl;
        cout << "N.B. The '-gt' flag cannot be used in conjunction with other flags" << endl;
        exit(-1);
    }

    // Parse command line arguments
    const char* filename = argv[1];

    bool ret_play = false;
    bool is_gt = false;
    bool is_phrase = false;
    vector<string> query_terms;
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-play") ret_play = true;
        else if (arg == "-gt") {
            is_gt = true;
            for (++i; i < argc; i++) {
                arg = argv[i];
                query_terms.push_back(arg);
            }
        }
        else if (arg == "-phrase") is_phrase = true;
        else query_terms.push_back(arg);
    }
    if (is_gt && (ret_play || is_phrase)) {
        cout << "The '-gt' flag cannot be used in conjunction with other flags" << endl;
        exit(-1);
    }


    // Read in file
    std::ifstream json_file (filename);
    if (!json_file.is_open()) {
        cout << "file could not be opened" << endl;
        exit(-1);
    }

    nlohmann::json j;
    try { j = nlohmann::json::parse(json_file); }
    catch (nlohmann::json::parse_error &e) {
        cout << e.what() << endl;
        exit(-1);
    }

    json_file.close();

    // Create inverted list
    for (auto &play : j["corpus"]) {
        string playId = play["playId"];
        string sceneId = play["sceneId"];
        doc_info.emplace_back(sceneId, playId);

        int docId = play["sceneNum"];
        string text = play["text"];

        int pos = 0;
        while (text.length() != 0) {
            // Check if first char of string is ' '
            if (text[0] == ' ') {
                text = text.substr(1, string::npos);
                continue;
            }

            int end_pos = (int) text.find(' ');
            string term = text.substr(0, end_pos);

            // Add term to inverted list
            if (!inverted_list.count(term)) inverted_list.insert(std::make_pair(term, new Postings()));
            inverted_list[term]->add_instance(docId, pos++);

            if (end_pos == string::npos) break;
            text = text.substr(end_pos + 1, string::npos);
        }
    }

    // Get queries
    std::unordered_set<int> docId_matches;
    if (is_gt) {
        unordered_map<int, int> term_freq;
        int i = 0;

        // Get max word count of every doc for every term that is to be greater
        for (; i < query_terms.size(); i++) {
            if (query_terms[i] == "-gt") {
                i++;
                break;
            }
            unordered_map<int, vector<int>> curr = *inverted_list[query_terms[i]]->get_postings_list();
            for (auto &item : curr) {
                int docId = item.first;
                int freq = (int) item.second.size();
                if (!term_freq.count(docId)) term_freq.insert(std::make_pair(docId, freq));
                else if (term_freq[docId] < freq) term_freq[docId] = freq;
            }
        }

        // for terms on the lesser, if they are not in the map, they automatically lose
        // If "lesser" terms have higher freq than "greater" term, remove doc from list
        for (; i < query_terms.size(); i++) {
            unordered_map<int, vector<int>> curr = *inverted_list[query_terms[i]]->get_postings_list();
            for (auto &item : curr) {
                int docId = item.first;
                int freq = (int) item.second.size();
                if (!term_freq.count(docId)) continue;
                else if (term_freq[docId] <= freq) term_freq[docId] = 0;
            }
        }

        // Add every "key" remaining in map to docId matches if its value is non-zero
        for (auto &item : term_freq) {
            if (item.second != 0) docId_matches.insert(item.first);
        }

    } else if (is_phrase) {
        // Get postings for every term in the phrase
        auto *postings = new vector<unordered_map<int, vector<int>>*>();
        postings->reserve(query_terms.size());
        for (const string &query : query_terms) { postings->push_back(inverted_list[query]->get_postings_list()); }

        unordered_map<int, vector<int>> term0_posting = *(*postings)[0]; // Need to dereference postings and the map it contains
        for (auto &item : term0_posting) {
            // Check that every term has this docId in their postings list
            int docId = item.first;
            bool hasDoc = true;
            for (int i = 1; i < postings->size(); i++) {
                // If docId not found w/in posting list
                if (!(*postings)[i]->count(docId)) {
                    hasDoc = false;
                    break;
                }
            }
            if (!hasDoc) continue;

            // Check for sequential positions within this doc, always starting at the first term
            vector<int> positions = item.second;
            for (auto pos : positions) {
                bool hasPhrase = true;
                int x = pos + 1;
                for (int i = 1; i < postings->size(); i++) {
                    vector<int> curr = (*(*postings)[i])[docId];
                    if (std::find(curr.begin(), curr.end(), x++) == curr.end()) {
                        hasPhrase = false;
                        break;
                    }
                }
                if (hasPhrase) {
                    docId_matches.insert(docId);
                    break;
                }
            }
        }
    } else {
        // Get all docId's where there is a match
        for (auto &arg : query_terms) {
            unordered_map<int, vector<int>> *posting = inverted_list[arg]->get_postings_list();
            for (auto &item : *posting) {
                docId_matches.insert(item.first);
            }
        }
    }

    // Check return type and compile list accordingly
    std::set<string> ret_list;
    if (ret_play) { for (int docId : docId_matches) ret_list.insert(doc_info[docId].second); }
    else { for (int docId : docId_matches) ret_list.insert(doc_info[docId].first); }

    std::ofstream output_file ("output.txt");
    for (const string& id : ret_list) { output_file << id << endl; }
    output_file.close();
}