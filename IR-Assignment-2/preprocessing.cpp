#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include <dirent.h>
#include <sys/stat.h>
#include "include/json.hpp"

using namespace std;
using json = nlohmann::json;

//  Porter Stemmer

static bool isVowel(const string& w, int i) {
    char c = w[i];
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') return true;
    if (c == 'y' && i > 0) return !isVowel(w, i - 1);
    return false;
}

static int measure(const string& s) {
    int m = 0; bool inV = false;
    for (int i = 0; i < (int)s.size(); ++i) {
        if (isVowel(s, i)) { inV = true; }
        else if (inV) { ++m; inV = false; }
    }
    return m;
}

static bool hasVowel(const string& s) {
    for (int i = 0; i < (int)s.size(); ++i)
        if (isVowel(s, i)) return true;
    return false;
}

static bool endsCVC(const string& w) {
    int n = w.size(); if (n < 3) return false;
    char c3 = w[n - 1];
    return !isVowel(w, n-3) && isVowel(w, n-2) && !isVowel(w, n-1)
           && c3 != 'w' && c3 != 'x' && c3 != 'y';
}

static bool hasSuffix(const string& w, const string& sfx, string& stem) {
    if (w.size() < sfx.size()) return false;
    if (w.compare(w.size() - sfx.size(), sfx.size(), sfx) == 0) {
        stem = w.substr(0, w.size() - sfx.size());
        return true;
    }
    return false;
}

string porterStem(const string& word) {
    string w = word;
    if (w.size() <= 2) return w;
    string stem;

    if      (hasSuffix(w, "sses", stem)) w = stem + "ss";
    else if (hasSuffix(w, "ies",  stem)) w = stem + "i";
    else if (!hasSuffix(w, "ss", stem) && hasSuffix(w, "s", stem)) w = stem;

    bool extra = false;
    if      (hasSuffix(w, "eed", stem)) { if (measure(stem) > 0) w = stem + "ee"; }
    else if (hasSuffix(w, "ed",  stem)) { if (hasVowel(stem)) { w = stem; extra = true; } }
    else if (hasSuffix(w, "ing", stem)) { if (hasVowel(stem)) { w = stem; extra = true; } }

    if (extra) {
        string s2;
        if      (hasSuffix(w, "at", s2)) w += "e";
        else if (hasSuffix(w, "bl", s2)) w += "e";
        else if (hasSuffix(w, "iz", s2)) w += "e";
        else {
            int n = w.size();
            if (n > 1 && w[n-1] == w[n-2]
                && !(w[n-1]=='l' || w[n-1]=='s' || w[n-1]=='z')
                && !isVowel(w, n-1))
                w = w.substr(0, n-1);
            else if (measure(w) == 1 && endsCVC(w))
                w += "e";
        }
    }

    if (hasSuffix(w, "y", stem) && hasVowel(stem)) w = stem + "i";

    struct { const char* s; const char* r; } s2[] = {
        {"ational","ate"},{"tional","tion"},{"enci","ence"},{"anci","ance"},
        {"izer","ize"},{"abli","able"},{"alli","al"},{"entli","ent"},{"eli","e"},
        {"ousli","ous"},{"ization","ize"},{"ation","ate"},{"ator","ate"},
        {"alism","al"},{"iveness","ive"},{"fulness","ful"},{"ousness","ous"},
        {"aliti","al"},{"iviti","ive"},{"biliti","ble"},
    };
    for (int i = 0; i < 20; ++i)
        if (hasSuffix(w, s2[i].s, stem) && measure(stem) > 0)
            { w = stem + s2[i].r; break; }

    struct { const char* s; const char* r; } s3[] = {
        {"icate","ic"},{"ative",""},{"alize","al"},{"iciti","ic"},
        {"ical","ic"},{"ful",""},{"ness",""},
    };
    for (int i = 0; i < 7; ++i)
        if (hasSuffix(w, s3[i].s, stem) && measure(stem) > 0)
            { w = stem + s3[i].r; break; }

    const char* s4[] = {
        "al","ance","ence","er","ic","able","ible","ant","ement","ment",
        "ent","ion","ou","ism","ate","iti","ous","ive","ize"
    };
    for (int i = 0; i < 19; ++i)
        if (hasSuffix(w, s4[i], stem)) {
            if (string(s4[i]) == "ion") {
                if (!stem.empty()
                    && (stem[stem.size()-1]=='s' || stem[stem.size()-1]=='t')
                    && measure(stem) > 1)
                    { w = stem; break; }
            } else if (measure(stem) > 1)
                { w = stem; break; }
        }

    if (hasSuffix(w, "e", stem)) {
        int m = measure(stem);
        if (m > 1) w = stem;
        else if (m == 1 && !endsCVC(stem)) w = stem;
    }

    { int n = w.size();
      if (n > 1 && w[n-1]=='l' && w[n-2]=='l' && measure(w) > 1)
          w = w.substr(0, n-1); }

    return w;
}

static vector<string> extractWords(const string& line) {
    vector<string> words; string cur;
    for (char c : line) {
        if (isalpha((unsigned char)c)) cur += c;
        else if (!cur.empty()) { words.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) words.push_back(cur);
    return words;
}

static int getNumber(const string& filename) {
    auto pos = filename.find('_');
    if (pos == string::npos) return 0;
    auto dot = filename.find('.', pos);
    string num = filename.substr(pos + 1,
                    dot == string::npos ? string::npos : dot - pos - 1);
    try { return stoi(num); } catch (...) { return 0; }
}

static vector<string> listFiles(const string& dirPath) {
    vector<string> files;
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) { cerr << "Cannot open directory: " << dirPath << "\n"; return files; }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name == "." || name == "..") continue;

        if (name.size() < 4 || name.substr(name.size() - 4) != ".txt") continue;

        string fullPath = dirPath + "/" + name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            files.push_back(name);
    }
    closedir(dir);
    return files;
}

int main() {

    set<string> stopWords;
    {
        ifstream f("Stopword-List.txt");
        if (!f.is_open()) { cerr << "Cannot open Stopword-List.txt\n"; return 1; }
        string line;
        while (getline(f, line)) {

            auto first = line.find_first_not_of(" \t\r\n");
            if (first == string::npos) continue;
            line = line.substr(first, line.find_last_not_of(" \t\r\n") - first + 1);
            if (!line.empty())
                stopWords.insert(porterStem(line)); 
        }
    }

    string dirPath = "Trump Speechs";
    vector<string> filenames = listFiles(dirPath);
    sort(filenames.begin(), filenames.end(), [](const string& a, const string& b) {
        return getNumber(a) < getNumber(b);
    });

    int N = (int)filenames.size();
    cout << "Found " << N << " documents.\n";
    if (N == 0) { cerr << "No documents found.. Check directory path.\n"; return 1; }

    map<string, map<int, int>>  tfRaw;
    map<string, string>         docMap; 

    for (int docId = 0; docId < N; ++docId) {
        docMap[to_string(docId)] = filenames[docId];

        string fullPath = dirPath + "/" + filenames[docId];
        ifstream f(fullPath);
        if (!f.is_open()) { cerr << "Cannot open " << fullPath << "\n"; continue; }

        string line;
        while (getline(f, line)) {
            auto words = extractWords(line);
            for (auto& word : words) {
                // Case-fold
                string w = word;
                transform(w.begin(), w.end(), w.begin(), ::tolower);
                // Stem
                string stemmed = porterStem(w);
                // Stop-word filter (compare stemmed forms)
                if (stopWords.count(stemmed)) continue;
                tfRaw[stemmed][docId]++;
            }
        }
    }
    cout << "Indexed " << tfRaw.size() << " unique terms.\n";

    // df[term] = number of documents that contain term
    map<string, int> df;
    for (auto& kv : tfRaw)
        df[kv.first] = (int)kv.second.size();

    map<string, map<int, double>> tfidf;
    map<int, double> docNorms;

    for (auto& kv : tfRaw) {
        const string& term = kv.first;
        double idf = log((double)N / (double)df[term]);

        for (auto& dv : kv.second) {
            int    docId = dv.first;
            double tf    = (double)dv.second;
            double score = tf * idf;
            tfidf[term][docId]  = score;
            docNorms[docId]    += score * score;
        }
    }
    for (auto& kv : docNorms)
        kv.second = sqrt(kv.second);

    {
        json j = json::object();
        for (auto& kv : tfRaw) {
            j[kv.first] = json::object();
            for (auto& dv : kv.second)
                j[kv.first][to_string(dv.first)] = dv.second;
        }
        ofstream out("tf_index.json");
        out << j.dump(2) << "\n";
    }

    {
        json j = json::object();
        for (auto& kv : df) j[kv.first] = kv.second;
        ofstream out("df_index.json");
        out << j.dump(2) << "\n";
    }

    {
        json j = json::object();
        for (auto& kv : tfidf) {
            j[kv.first] = json::object();
            for (auto& dv : kv.second)
                j[kv.first][to_string(dv.first)] = dv.second;
        }
        ofstream out("tfidf_index.json");
        out << j.dump(2) << "\n";
    }

    {
        json j = json::object();
        for (auto& kv : docMap) j[kv.first] = kv.second;
        ofstream out("doc_map.json");
        out << j.dump(2) << "\n";
    }

    {
        json j = json::object();
        for (auto& kv : docNorms)
            j[to_string(kv.first)] = kv.second;
        ofstream out("doc_norms.json");
        out << j.dump(2) << "\n";
    }

    cout << "\nPreprocessing complete.  N=" << N
         << " docs,  " << tfRaw.size() << " unique terms.\n";
    return 0;
}