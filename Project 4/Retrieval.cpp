/* Citations:
 * https://stackoverflow.com/questions/3450860/check-if-a-stdvector-contains-a-certain-object
 * https://www.geeksforgeeks.org/sorting-a-map-by-value-in-c-stl/
 * https://stackoverflow.com/questions/15056406/append-to-a-file-with-fstream-instead-of-overwriting
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cmath>
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

// Maps scene_count[sceneId] = count;
map<string, int> scene_count;

// Command Line Arguments ([k1, k2, b] for BM25 and [mu] for QL)
vector<double> params;

// Set of queries to process
vector<vector<string>> Q = {vector<string>{"the", "king", "queen", "royalty"},
                            vector<string>{"servant", "guard", "soldier"},
                            vector<string>{"hope", "dream", "sleep"},
                            vector<string>{"ghost" , "spirit"},
                            vector<string>{"fool", "jester", "player"},
                            vector<string>{"to", "be", "or", "not", "to", "be"}
};

// Run tag - Only net-ID portion included here
string run_tag = "vslam";


/********* API's *********/
// Returns the frequency of the term
int get_term_freq(const string& term) { return inverted_list[term]->get_term_count();}

// Returns the number of docs this term occurs in
int get_doc_freq(const string& term) { return inverted_list[term]->get_doc_count(); }

// Returns the size of the vocab collection
int get_collection_size() { return (int) inverted_list.size(); }

// Returns the length of the document
int get_doc_length(int docId) { return scene_count[doc_info[docId].first]; }

// Returns the number of documents
int get_num_docs() { return (int) doc_info.size(); }

// Returns the avg length of a play
double get_avg_scene_length() {
    double sum = 0;
    for (const auto &scene : scene_count) { sum += scene.second; }
    return sum / (double) scene_count.size();
}

// Returns the shortest scene and length of that scene
pair<string, int> get_shortest_scene() {
    return std::make_pair(scene_count.begin()->first, scene_count.begin()->second);
}

// Returns the longest scene and length of that scene
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

// Builds the index given the file
void build_index(const char* filename) {
    // Read in file
    std::ifstream json_file (filename);
    if (!json_file.is_open()) {
        cout << "File could not be opened" << endl;
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
            if (!inverted_list.count(term)) { inverted_list.insert(std::make_pair(term, new Postings())); }
            inverted_list[term]->add_instance(docId, pos++);

            if (end_pos == string::npos) break;
            text = text.substr(end_pos + 1, string::npos);
        }
        scene_count[sceneId] = pos;
        if (!play_count.count(playId)) { play_count.insert(std::make_pair(playId, 0)); }
        play_count[playId] += pos;
    }
}

// Cmp function for sorting the map
bool cmp(pair<int, double>& a, pair<int, double>& b) { return a.second > b.second; }

// Returns a vector of doc's sorted by value
vector<pair<int, double>> map_sort(unordered_map<int, double>& M) {
    vector<pair<int, double>> vect;

    vect.reserve(M.size());
    for (auto& it : M) { vect.emplace_back(it); }

    // Sort using comparator function
    sort(vect.begin(), vect.end(), cmp);

    return vect;
}

// Calculate BM25 scores
void calculate_BM25() {
    // Edit run-tag to show it's for the BM25 model, include values k1, k2, and b for reference
    run_tag += "_BM-" + std::to_string(params[0]) + "-" + std::to_string(params[1]) + "-" + std::to_string(params[2]);

    // Clear bm25.trecrun file
    std::ofstream output ("bm25.trecrun");
    output << "";
    output.close();

    // Store the score as scores[docId] = score inside hash table
    unordered_map<int, double> scores;

    // Save values that will be used later
    // Command line args
    double k1 = params[0];
    double k2 = params[1];
    double b = params[2];
    // Size of the document collection in docs
    int N = (int) doc_info.size();
    // Number of documents that contain the term i
    int ni;
    // Calculated using K = k_1((1 - b) + b * doc_length/avdl)
    double K;
    // Frequency of term i within document D
    int fi;
    // Frequency of the term within the query
    int qf;
    // Average document length
    double avdl = get_avg_scene_length();

    // Term at a time retrieval
    for (int query_num = 0; query_num < Q.size(); query_num++) {  // Query from vector Q
        // Contains all the terms in the query
        vector<string> query = Q[query_num];

        // Set values for IDF calculation

        for (string &term : query) {
            // Postings list for each term in the query
            unordered_map<int, vector<int>> *term_postings = inverted_list[term]->get_postings_list();

            ni = (int) term_postings->size();
            qf = (int) std::count(query.begin(), query.end(), term);

            // Calculate IDF component since it will be the same for this term
            double idf = log((N - ni + 0.5)/(ni + 0.5));

            // Iterate over documents that contain this term
            for (auto &item : *term_postings) {
                K = k1 * ((1 - b) + (b * get_doc_length(item.first)/avdl));
                fi = (int) item.second.size();

                double score = idf * (fi * (k1+1)/(K+fi)) * (qf * (k2 + 1)/(k2 + qf));
                if (!scores.count(item.first)) { scores.insert(std::make_pair(item.first, 0)); }
                scores[item.first] += score;
            }
        }

        // Sort data
        vector<pair<int, double>> final_scores = map_sort(scores);
        scores.clear();

        // Write to file
        int num = query_num + 1;
        std::ofstream trec_output ("bm25.trecrun", std::ios_base::app);
        int rank = 0;
        for (auto &item : final_scores) {
            trec_output << "Q" << num << " skip " << doc_info[item.first].first << " " << ++rank << " " << item.second << " " << run_tag << endl;
        }
        trec_output.close();
    }

}

// Calculate QL Scores
void calculate_QL() {
    run_tag += "_QL-" + std::to_string(params[0]);

    // Clear output file
    std::ofstream output ("ql.trecrun");
    output << "";
    output.close();

    // Maps scores[docId] = score
    unordered_map<int, double> scores;

    // Got from command line
    double mu = params[0];
    // |C| = Total # of word occurrences in the collection
    int C = 0;
    for (auto &item : inverted_list) { C += item.second->get_term_count(); }

    // Experimental - Should be correct
    for (int query_num = 0; query_num < Q.size(); query_num++) {
        vector<string> query = Q[query_num];

        std::unordered_set<int> docs;
        // Iterate over all documents. If a doc contains a single term, we calculate the other terms for that doc too
        for (int doc = 0; doc < doc_info.size(); doc++) {
            for (auto &term : query) {
                if (!inverted_list[term]->get_postings_list()->count(doc)) {
                    docs.insert(doc);
                    break;
                }
            }
        }

        // Iterate over all terms
        for (auto &term : query) {
            // c_qi = # of times a query word occurs in the collection of documents
            int Cqi = get_term_freq(term);

            unordered_map<int, vector<int>> *term_postings = inverted_list[term]->get_postings_list();

            for (auto &doc : docs) {
                // Document length
                int D = get_doc_length(doc);
                // Frequency of the term in the given doc
                int fqiD;
                if (!term_postings->count(doc)) { fqiD = 0; }
                else { fqiD = (int) term_postings[doc].size(); }
                double score = log((fqiD + (mu * Cqi/C))/(D + mu));
                if (!scores.count(doc)) { scores.insert(std::make_pair(doc, 0)); }
                scores[doc] += score;
            }

        }
        // Sort data
        vector<pair<int, double>> final_scores = map_sort(scores);
        scores.clear();

        // Write to file
        int num = query_num + 1;
        std::ofstream trec_output ("ql.trecrun", std::ios_base::app);
        int rank = 0;
        for (auto &item : final_scores) {
            trec_output << "Q" << num << " skip " << doc_info[item.first].first << " " << ++rank << " " << item.second << " " << run_tag << endl;
        }
        trec_output.close();
    }

}

int main(int argc, char **argv) {
    if (argc < 2) {
        cout << "Usage: ./indexer <file.json> { -QL mu | -BM25 k1 k2 b }" << endl;
        exit(-1);
    }

    build_index((const char*) argv[1]);

    if ((string) argv[2] == "-QL") {
        params.push_back(atof(argv[3]));
        calculate_QL();
    }
    else if ((string) argv[2] == "-BM25") {
        for (int i = 3; i < 6; i++) { params.push_back(atof(argv[i])); }
        calculate_BM25();
    }
    else {
        cout << "Enter valid query processing model" << endl;
        exit(-1);
    }

    return 0;
}

