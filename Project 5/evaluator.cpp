/* Sources:
 * https://stackoverflow.com/questions/11719538/how-to-use-stringstream-to-separate-comma-separated-strings
 * https://www.geeksforgeeks.org/sorting-a-map-by-value-in-c-stl/
 */
//#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cmath>

using std::cout;
using std::endl;

/* Data Structures ---------------------------------------------------------------------------------------------------*/
// Maps ranks[query_num] -> rank[i] = rank_i
std::unordered_map<int, std::vector<int>> ranks;
// Maps rankings[query_num] -> ranked_list[rank] = <docId, score>
std::unordered_map<int, std::unordered_map<int, std::pair<std::string, double>>> rankings;
// Maps relevancy_list[query_num] -> rel_list[docId] = relevancy_score
std::unordered_map<int, std::unordered_map<std::string, int>> relevancy_list;


/* Forward Declarations ----------------------------------------------------------------------------------------------*/
void get_query_relevance(const char* filename);
void get_rankings(const char* filename);
double normalized_discounted_cumulative_gain(int query_num, int k);
double reciprocal_rank(int query_num);
double precision(int query_num, int k);
double average_precision(int query_num, int k);
double recall(int query_num);
double F1(double R, double P);

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Usage: ./evaluate qrels filename" << endl;
        cout << "e.g.: ./evaluate qrels ql.trecrun" << endl;
        return -1;
    }

    // Read files into datastructures
    get_query_relevance(argv[1]);
    get_rankings(argv[2]);



    // Perform evaluations
    double ndcg = 0;
    double mrr = 0;
    double p5 = 0;
    double p20 = 0;
    double recall20 = 0;
    double f = 0;
    double map = 0;

    for (auto query : rankings) {
        ndcg += normalized_discounted_cumulative_gain(query.first, 10);
        mrr += reciprocal_rank(query.first);
        p5 += average_precision(query.first, 5);
        p20 += average_precision(query.first, 20);
        map += average_precision(query.first, -1);
        recall20 += recall(query.first);
    }

    int num_queries = rankings.size();
    ndcg /= num_queries;
    mrr /= num_queries;
    p5 /= num_queries;
    p20 /= num_queries;
    map /= num_queries;
    recall20 /= num_queries;
    f = F1(recall20, p20);

    // Write evaluations to a file
    std::ofstream outfile("output.metrics");
    outfile << argv[2] << " NDCG " << ndcg << endl;
    outfile << argv[2] << " MRR " << mrr << endl;
    outfile << argv[2] << " P@5 " << p5 << endl;
    outfile << argv[2] << " P@20 " << p20 << endl;
    outfile << argv[2] << " recall@20 " << recall20 << endl;
    outfile << argv[2] << " F1@20 " << f << endl;
    outfile << argv[2] << " MAP " << map << endl;
    outfile.close();

}

/* Functions Definitions -------------------------------------------------------------------------------------------- */
// Helper functions
double min(double a, double b) { return (a > b ? b : a); }
double max(double a, double b) { return (a > b ? a : b); }
bool cmp(std::pair<std::string, int> a, std::pair<std::string, int> b) { return a.second < b.second; }
std::vector<std::pair<std::string, int>> map_sort(std::unordered_map<std::string, int> m) {
    std::vector<std::pair<std::string, int>> vect;
    for (auto it : m) { vect.push_back(it); }
    std::sort(vect.begin(), vect.end(), cmp);
    return vect;
}
void get_query_relevance(const char* filename) {
    std::ifstream infile(filename);
    std::string line, tok, docId;
    int index = 0, query_num;
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        for (int i = 0; i < 4; i++) {
            ss >> tok;
            switch(i) {
                case 0: // QueryNum
                    // If query_num (eg 301) not in map, add to map
                    query_num = std::stoi(tok);
                    if (!relevancy_list.count(query_num))
                        relevancy_list.insert(std::make_pair(query_num, std::unordered_map<std::string, int>{}));
                    break;
                case 2: // docId
                    docId = tok;
                    break;
                case 3:
                    if (std::stoi(tok) > 0) {
                        relevancy_list[query_num].insert(std::make_pair(docId, std::stoi(tok)));
                    }
                    break;
            }
        }
    }
    infile.close();
}
void get_rankings(const char* filename) {
    std::ifstream infile(filename);
    std::string line, tok, docId;
    int index = 0, rank, query_num;
    double score;
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        for (int i = 0; i < 6; i++) {
            ss >> tok;
            switch(i) {
                case 0: //queryNum
                    query_num = std::stoi(tok);
                    // Add query_num to rankings and ranks
                    if (!rankings.count(query_num)) {
                        rankings.insert(std::make_pair(query_num, std::unordered_map<int, std::pair<std::string, double>>{}));
                    }
                    if (!ranks.count(query_num)) {
                        ranks.insert(std::make_pair(query_num, std::vector<int>{}));
                    }
                    break;
                case 2: // docId
                    docId = tok;
                    break;
                case 3: // Rank
                    rank = std::stoi(tok);
                    break;
                case 4: // Score
                    score = std::stod(tok);
                    break;
            }
        }
        ranks[query_num].push_back(rank);
        rankings[query_num].insert(std::make_pair(rank, std::make_pair(docId, score)));
    }
    infile.close();
}

// Evaluation Functions
double normalized_discounted_cumulative_gain(int query_num, int k) {
    std::unordered_map<int, std::pair<std::string, double>> ranked_list = rankings[query_num];
    std::unordered_map<std::string, int> rel_list = relevancy_list[query_num];
    std::vector<int> rank = ranks[query_num];
    std::sort(rank.begin(), rank.end());
    std::vector<std::pair<std::string, int>> sorted_rel_list = map_sort(rel_list);
    k = min(k, rank.size());
    double idcg = 0;
    idcg = sorted_rel_list[0].second;
    for (int i = 1; i < k; i++) { idcg += sorted_rel_list[i].second/log2(i + 1); }
    double dcg = 0;
    if (rel_list.count(ranked_list[rank[0]].first)) { dcg = rel_list[ranked_list[rank[0]].first]; }
    for (int i = 1; i < k; i++) {
        if (rel_list.count(ranked_list[rank[i]].first)) {
            dcg += rel_list[ranked_list[rank[i]].first]/log2(i + 1);
        }
    }
    return dcg/idcg;
}
double reciprocal_rank(int query_num) {
    std::unordered_map<int, std::pair<std::string, double>> ranked_list = rankings[query_num];
    std::unordered_map<std::string, int> rel_list = relevancy_list[query_num];
    std::vector<int> rank = ranks[query_num];
    std::sort(rank.begin(), rank.end());
    for (int i = 0; i < rank.size(); i++) {
        if (rel_list.count(ranked_list[rank[i]].first)) { return 1/(i + 1); }
    }
    return 0.0;
}
double precision(int query_num, int k) {
    std::unordered_map<int, std::pair<std::string, double>> ranked_list = rankings[query_num];
    std::unordered_map<std::string, int> rel_list = relevancy_list[query_num];
    std::vector<int> rank = ranks[query_num];
    std::sort(rank.begin(), rank.end());
    k = min(k, rank.size());
    int rel_seen = 0;
    for (int i = 0; i < k; i++) {
        if (rel_list.count(ranked_list[rank[i]].first)) { rel_seen++; }
    }
    return rel_seen/k;
}
double average_precision(int query_num, int k) {
    std::unordered_map<int, std::pair<std::string, double>> ranked_list = rankings[query_num];
    std::unordered_map<std::string, int> rel_list = relevancy_list[query_num];
    std::vector<int> rank = ranks[query_num];
    std::sort(rank.begin(), rank.end());
    if (k == -1) k = rank.size();
    else k = min(k, rank.size());
    double ap = 0;
    int rel_seen = 0;
    for (int i = 0; i < k; i++) {
        if (rel_list.count(ranked_list[rank[i]].first)) {
            rel_seen++;
            ap += rel_seen/(i + 1);
        }
    }
    ap /= rel_list.size();
    return ap;
}
double recall(int query_num) {
    std::unordered_map<int, std::pair<std::string, double>> ranked_list = rankings[query_num];
    std::unordered_map<std::string, int> rel_list = relevancy_list[query_num];
    std::vector<int> rank = ranks[query_num];
    std::sort(rank.begin(), rank.end());
    double intersect = 0;
    cout << rel_list.size() << endl;
    for (int i = 0; i < rank.size(); i++) {
        if (rel_list.count(ranked_list[rank[i]].first)) { intersect++; }
        if (query_num == 450)
            cout << intersect << endl;
    }
    return intersect/rel_list.size();
}
double F1(double R, double P) { return 2*R*P/(R + P); }
