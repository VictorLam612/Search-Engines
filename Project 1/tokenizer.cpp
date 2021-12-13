#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_map>

using namespace std;

set<string> stopwords;
set<char> vowels = {'a', 'e', 'i', 'o', 'u'};
vector<string> input;
unordered_map<string, int> token_counter;
vector<pair<string, int>> token_freq;

void getStopwords(const char* filename) {
    ifstream stopwords_stream (filename);
    string word;
    while(getline(stopwords_stream, word, '\n')) stopwords.insert(word);
    stopwords_stream.close();
}

void getInput(const char* filename) {
    ifstream input_stream (filename);
    string word;
    while(getline(input_stream, word, ' ')) input.push_back(word);
    input_stream.close();
}

string abbreviate(string& s) {
    bool ab = false;
    int curr = 0;
    while (curr < s.length()) {
        if (s[curr] == '.' && s[curr + 2] == '.') {
            s.erase(s.begin() + curr);
            ab = true;
        } else if (ab) {
            s.erase(s.begin() + curr);
            ab = false;
        }
        curr++;
    }
    return s;
}

string removePunc(string s) {
    for (int i = 0; i < s.length(); i++) {
        if (!isalnum(s[i])) {
            if (i + 1 < s.length()) input.push_back(s.substr(i + 1));
            s = s.substr(0, i);
            i--;
        }
    }
    return s;
}

bool hasVowel(string s, int n) {
    for (int i = 0; i < s.length() - n; i++) if (vowels.count(s[i])) return true;
    return false;
}

string adjustWord(string s) {
    if (!hasVowel(s, 0)) return s;
    string foo = s.substr(s.length() - 2);
    if (foo == "at" || foo == "bl" || foo == "iz") {
        s.append("e");
        return s;
    }

    set<string> invalidDoubles = {"ll", "ss", "zz"};
    if (foo[0] == foo[1] && !invalidDoubles.count(foo)) {
        return s.substr(0, s.length() - 1);
    } else if (s.length() < 4) {
        s.append("e");
        return s;
    }
    return s;
}

string porterStem(string word) {
    // Step 1a
    int l = word.length();
    if (l > 4 && word.substr(l - 4) == "sses") {
        word = word.substr(0, l - 2);
    } else if (l > 3) {
        if (word.substr(l - 3) == "ies" || word.substr(l - 3) == "ied") {
            if (l > 4) word = word.substr(0, l - 2);
            else word = word.substr(0, l - 1);
        } else if (l > 2 && word[l - 1] == 's' && hasVowel(word, 2)) {
            word = word.substr(0, l - 1);
        }
    } else if (word.substr(l - 2) == "us" || word.substr(l - 2) == "ss") {
        // Do nothing
    } else if (l > 2 && word[l - 1] == 's' && hasVowel(word, 2)) {
        word = word.substr(0, l - 1);
    }

    // Step 1b
    l = word.length();
    if (l > 7 && word.substr(l - 5) == "eedly" && vowels.count(word[l - 7]) && !vowels.count(word[l - 6])) {
        word = word.substr(0, l - 3);
    } else if (l > 5 && word.substr(l - 3) == "eed" && vowels.count(word[l - 5]) && !vowels.count(word[l - 4])) {
        word = word.substr(0, l - 1);
    } else if (l > 5 && word.substr(l - 5) == "ingly") {
        word = adjustWord(word.substr(0,l - 5));
    } else if (l > 4 && word.substr(l - 4) == "edly") {
        word = adjustWord(word.substr(0,l - 4));
    } else if (l > 3 && word.substr(l - 3) == "ing") {
        word = adjustWord(word.substr(0,l - 3));
    } else if (l > 2 && word.substr(l - 2) == "ed") {
        word = adjustWord(word.substr(0, l - 2));
    }
    return word;
}

bool cmp(pair<string, int> a, pair<string, int> b) {
    return a.second > b.second;
}

void mapSort(unordered_map<string, int>& map) {
    // Copy key-value pair from Map to vector of pairs
    for (auto& it : map) token_freq.push_back(it);

    // Sort using comparator function
    sort(token_freq.begin(), token_freq.end(), cmp);
}

int min(int a, int b) {
    if (a < b) return a;
    return b;
}

int main(int argc, char **argv) {
    if (argc < 3) cout << "To run: ./Tokenization stopwords.txt text.txt" << endl;

    // Get stopwords and store them in set<string>
    getStopwords((const char*) argv[1]);

    ifstream input_stream ((const char*) argv[2]);
    ofstream vocab_growth_output ((const char*) "vocab_growth.csv");
    int collection_size = 0;

    string word;
    while(getline(input_stream, word, ' ')) {
        // Abbreviate
        word = abbreviate(word);
        // Remove Punctuation
        word = removePunc(word);
        // Lowercase
        for (int i = 0; i < word.length(); i++) word[i] = tolower(word[i]);
        // Stopword removal
        if (stopwords.find(word) != stopwords.end() || word.length() == 0 || (word.length() == 1 && !isalnum(word[0]))) continue;
        // Porter Stemming
        word = porterStem(word);
        // Increment collection size
        collection_size++;
        // Store in map
        if (token_counter.find(word) != token_counter.end()) token_counter[word]++;
        else  token_counter.insert(pair<string, int>(word, 1));
        // Write data to file
        vocab_growth_output << collection_size << "," << token_counter.size() << endl;

    }
    for (int i = 0; i < input.size(); i++) {
        word = input[i];
        // Remove Punctuation
        word = removePunc(word);
        // Lowercase
        for (int i = 0; i < word.length(); i++) word[i] = tolower(word[i]);
        // Stopword removal
        if (stopwords.find(word) != stopwords.end() || word.length() == 0 || (word.length() == 1 && !isalnum(word[0]))) continue;
        // Porter Stemming
        word = porterStem(word);
        // Increment collection size
        collection_size++;
        // Store in map
        if (token_counter.find(word) != token_counter.end()) token_counter[word]++;
        else  token_counter.insert(pair<string, int>(word, 1));
        // Write data point to file
        vocab_growth_output << collection_size << "," << token_counter.size() << endl;
    }
    input_stream.close();
    vocab_growth_output.close();

    // Get map items sorted by value
    mapSort(token_counter);

    // Write word and freq to files
    ofstream output_stream ((const char*) "terms.txt");
    int max = min(200, token_freq.size());
    for (int i = 0; i < max; i++) {
        output_stream << token_freq[i].first << " " << token_freq[i].second << endl;
    }
    output_stream.close();

    return 0;
}
