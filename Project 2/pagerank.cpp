/*
 * https://www.geeksforgeeks.org/sorting-a-map-by-value-in-c-stl/
 * https://stackoverflow.com/questions/11315854/input-from-command-line
 */

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <set>
#include <vector>
#include <cmath>

using namespace std;

// Inlink Data Structures
unordered_map<string, double> inlinks;

// PageRank Data Structures
unordered_map<string, set<string>> graph;           // Graph G in pseudocode
unordered_map<string, double> I;                    // Vector I in pseudocode
unordered_map<string, double> R;                    // Vector R in pseudocode
int norm = 0;


bool cmp(pair<string, double> a, pair<string, double> b) {
    return a.second > b.second;
}

vector<pair<string, double>> mapSort(unordered_map<string, double>& map) {
    vector<pair<string, double>> ret;
    for (auto& it : map) {
        ret.push_back(pair<string, double>(it.first, it.second));
    }

    sort(ret.begin(), ret.end(), cmp);
    return ret;
}

int min(int a, int b) {
    if (a < b) return a;
    return b;
}

int main(int argc, char** argv) {
    if (argc < 2) cout << "To run: ./pagerank links.srt (double)lambda (double)tau" << endl;

    // Get command line arguments
    const char* filename = (const char*) argv[1];
    double lambda = atof(argv[2]);
    double tau = atof(argv[3]);

    // Read links from file
    ifstream input_stream (filename);
    string line, page, link;
    while (getline(input_stream, line, '\n')) {
        int mid = line.find('\t');
        page = line.substr(0, mid);
        link = line.substr(mid + 1);

        // Put into inlink map
        if (inlinks.count(link) == 0) inlinks.insert(pair<string, int>(link, 1));
        else inlinks[link]++;

        // Put source into PageRank graph
        if (graph.count(page) == 0) graph.insert(pair<string, set<string>>(page, set<string>{link}));
        else graph[page].insert(link);

        // Put target into PageRank graph
        if (graph.count(link) == 0) graph.insert(pair<string, set<string>>(link, set<string>{}));

    }
    input_stream.close();


    // Print top 75 pages ranked by inlinks to file "inlink.txt"
    ofstream inlink("inlink.txt");
    vector<pair<string, double>> inlink_ranks = mapSort(inlinks);
    int limit = min(inlinks.size(), 75);
    for (int i = 0; i < limit; i++) {
        inlink << inlink_ranks[i].first << " " << i + 1 << " " << inlink_ranks[i].second << endl;
    }
    inlink.close();


    // Calculating PageRank
    // Put every page into I & R
    for (auto& it : graph) {
        I[it.first] = 0;
        R[it.first] = 0;
    }

    // Set initial likelihood of being on each page
    for (auto& it : I) I[it.first] = 1.0/graph.size();

    // While not converged, update PageRanks
    while (norm < tau) {
        // Accumulator
        double to_add = 0;

        // Account for random surfer
        for (auto& it: graph) R[it.first] = lambda/graph.size();

        // Adjust target page pageranks
        // Add to accumulator for pages with no outlinks
        for (auto& it : graph) {
            if (it.second.size() > 0) {
                set<string> targets = it.second;
                for (auto& q : targets) {
                    // Add probability of coming to this page from previous page
                    R[q] += (1 - lambda) * I[q]/targets.size();
                }
            } else {
                // Accumulating sum of probabilities for pages w/ no outlinks
                to_add += (1 - lambda) * I[it.first]/graph.size();
            }
        }

        // Adding probability of being on page with no outlink
        // Calculating norm
        // Setting I = R for next iteration
        for (auto& it : graph) {
            R[it.first] += to_add;
            norm += abs(I[it.first] - R[it.first]);
            I[it.first] = R[it.first];
        }
    }

    ofstream pagerank_output ("pagerank.txt");
    vector<pair<string, double>> pagerank = mapSort(R);
    limit = min(pagerank.size(), 75);
    for (int i = 0; i < limit; i++) {
        pagerank_output << pagerank[i].first << " " << i + 1 << " " << pagerank[i].second << endl;
    }
    pagerank_output.close();

    return 0;
}


