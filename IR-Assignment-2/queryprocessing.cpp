#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include "include/json.hpp"

using namespace std;
using json = nlohmann::json;

//  Porter Stemmer

static bool isVowel(const string& w, int i) {
    char c = w[i];
    if (c=='a'||c=='e'||c=='i'||c=='o'||c=='u') return true;
    if (c=='y' && i>0) return !isVowel(w, i-1);
    return false;
}
static int measure(const string& s) {
    int m=0; bool inV=false;
    for (int i=0;i<(int)s.size();++i) {
        if (isVowel(s,i)){inV=true;}
        else if (inV){++m;inV=false;}
    }
    return m;
}
static bool hasVowel(const string& s) {
    for (int i=0;i<(int)s.size();++i) if (isVowel(s,i)) return true;
    return false;
}
static bool endsCVC(const string& w) {
    int n=w.size(); if(n<3) return false; char c3=w[n-1];
    return !isVowel(w,n-3)&&isVowel(w,n-2)&&!isVowel(w,n-1)
           &&c3!='w'&&c3!='x'&&c3!='y';
}
static bool hasSuffix(const string& w, const string& sfx, string& stem) {
    if (w.size()<sfx.size()) return false;
    if (w.compare(w.size()-sfx.size(),sfx.size(),sfx)==0)
        { stem=w.substr(0,w.size()-sfx.size()); return true; }
    return false;
}
string porterStem(const string& word) {
    string w=word; if(w.size()<=2) return w; string stem;
    if      (hasSuffix(w,"sses",stem)) w=stem+"ss";
    else if (hasSuffix(w,"ies", stem)) w=stem+"i";
    else if (!hasSuffix(w,"ss",stem)&&hasSuffix(w,"s",stem)) w=stem;
    bool extra=false;
    if      (hasSuffix(w,"eed",stem)){if(measure(stem)>0) w=stem+"ee";}
    else if (hasSuffix(w,"ed", stem)){if(hasVowel(stem)){w=stem;extra=true;}}
    else if (hasSuffix(w,"ing",stem)){if(hasVowel(stem)){w=stem;extra=true;}}
    if (extra) {
        string s2;
        if      (hasSuffix(w,"at",s2)) w+="e";
        else if (hasSuffix(w,"bl",s2)) w+="e";
        else if (hasSuffix(w,"iz",s2)) w+="e";
        else { int n=w.size();
               if(n>1&&w[n-1]==w[n-2]&&!(w[n-1]=='l'||w[n-1]=='s'||w[n-1]=='z')&&!isVowel(w,n-1))
                   w=w.substr(0,n-1);
               else if(measure(w)==1&&endsCVC(w)) w+="e"; }
    }
    if (hasSuffix(w,"y",stem)&&hasVowel(stem)) w=stem+"i";
    struct{const char*s;const char*r;}s2[]={
        {"ational","ate"},{"tional","tion"},{"enci","ence"},{"anci","ance"},
        {"izer","ize"},{"abli","able"},{"alli","al"},{"entli","ent"},{"eli","e"},
        {"ousli","ous"},{"ization","ize"},{"ation","ate"},{"ator","ate"},
        {"alism","al"},{"iveness","ive"},{"fulness","ful"},{"ousness","ous"},
        {"aliti","al"},{"iviti","ive"},{"biliti","ble"},
    };
    for(int i=0;i<20;++i) if(hasSuffix(w,s2[i].s,stem)&&measure(stem)>0){w=stem+s2[i].r;break;}
    struct{const char*s;const char*r;}s3[]={
        {"icate","ic"},{"ative",""},{"alize","al"},{"iciti","ic"},
        {"ical","ic"},{"ful",""},{"ness",""},
    };
    for(int i=0;i<7;++i) if(hasSuffix(w,s3[i].s,stem)&&measure(stem)>0){w=stem+s3[i].r;break;}
    const char*s4[]={"al","ance","ence","er","ic","able","ible","ant","ement","ment",
                     "ent","ion","ou","ism","ate","iti","ous","ive","ize"};
    for(int i=0;i<19;++i)
        if(hasSuffix(w,s4[i],stem)){
            if(string(s4[i])=="ion"){
                if(!stem.empty()&&(stem[stem.size()-1]=='s'||stem[stem.size()-1]=='t')
                   &&measure(stem)>1){w=stem;break;}
            } else if(measure(stem)>1){w=stem;break;}
        }
    if(hasSuffix(w,"e",stem)){int m=measure(stem);if(m>1)w=stem;else if(m==1&&!endsCVC(stem))w=stem;}
    {int n=w.size();if(n>1&&w[n-1]=='l'&&w[n-2]=='l'&&measure(w)>1)w=w.substr(0,n-1);}
    return w;
}


static string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
static string trim(const string& s) {
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
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

map<string, map<string, double>> tfidfIndex;

// dfIndex[term] = #documents containing term
map<string, int> dfIndex;

// docNorms[docId_str] = L2 norm of document's TF-IDF vector
map<string, double> docNorms;

// docMap[docId_str] = filename
map<string, string> docMap;

// stopWords: stemmed forms of the stop-word list
set<string> stopWords;

// N: total number of documents in Trump Speechs
int N = 0;

struct DocScore {
    int    docId;
    double score;
    string filename;
};

vector<DocScore> vsmQuery(const string& rawQuery, double alpha = 0.005) {

    map<string, int> queryTF;
    for (auto& word : extractWords(rawQuery)) {
        string stemmed = porterStem(toLower(word));
        if (stopWords.count(stemmed)) continue;
        queryTF[stemmed]++;
    }
    if (queryTF.empty()) return {};

    map<string, double> queryVec;
    double queryNorm = 0.0;
    for (auto& kv : queryTF) {
        const string& term = kv.first;
        auto it = dfIndex.find(term);
        if (it == dfIndex.end()) continue;  // term not in Trump Speechs – skip

        double idf   = log((double)N / (double)it->second);
        double score = (double)kv.second * idf;
        queryVec[term] = score;
        queryNorm += score * score;
    }
    queryNorm = sqrt(queryNorm);
    if (queryNorm == 0.0) return {};

    map<int, double> dotProducts;
    for (auto& kv : queryVec) {
        auto it = tfidfIndex.find(kv.first);
        if (it == tfidfIndex.end()) continue;
        double qScore = kv.second;
        for (auto& dv : it->second) {
            int docId = stoi(dv.first);
            dotProducts[docId] += qScore * dv.second;
        }
    }

    vector<DocScore> results;
    for (auto& kv : dotProducts) {
        int    docId  = kv.first;
        double dot    = kv.second;
        auto   nrmIt  = docNorms.find(to_string(docId));
        double dNorm  = (nrmIt != docNorms.end()) ? nrmIt->second : 1.0;
        double cosine = dot / (queryNorm * dNorm);

        if (cosine >= alpha) {
            DocScore ds;
            ds.docId    = docId;
            ds.score    = cosine;
            auto dmIt   = docMap.find(to_string(docId));
            ds.filename = (dmIt != docMap.end()) ? dmIt->second : "unknown";
            results.push_back(ds);
        }
    }

    sort(results.begin(), results.end(), [](const DocScore& a, const DocScore& b) {
        return a.score > b.score;
    });

    return results;
}

void testQueries() {
    struct TestCase { string query; set<string> expected; };
    vector<TestCase> tests;

    auto addTest = [&](const string& q, initializer_list<const char*> ids) {
        TestCase tc; tc.query = q;
        for (auto id : ids) tc.expected.insert(id);
        tests.push_back(tc);
    };

    addTest("massive inflow of refugees",
            {"32","50","49","47","46","29","48","54","41","40","30","39",
             "12","52","37","44","31","38","20"});

    addTest("pakistan afghanistan",
            {"3","22","16","17","4","1"});

    addTest("Hillary Clinton",
            {"4","12","36","42","33","20","29","35","16","22","47","34","14",
             "21","45","46","41","11","3","51","53","40","37","17","48","39",
             "5","28","10","18","54","44","25","43","49","24","30","32","8",
             "50","9","26","7","19","52","31","27","6","2","38","1","55"});

    addTest("personnel policies",
            {"5","18","11","25","10","22","29","27","17"});

    addTest("united plane",
            {"38","1","25","41","48","29","3","17","49","20","4","52","13",
             "39","46","22","40","36","28","12","45","26","2","51","50","19",
             "24","5","47","31","35","21","37","55","9","33","44","54","34",
             "7","32","43","30","16","27","18","11","10","8"});

    addTest("develop solutions",
            {"38","23","2","17","32","18","51","16","39","35","21","27","33",
             "20","24","5","3","9","40","52","30"});

    addTest("developments praised",
            {"36","44","8"});

    addTest("muslims",
            {"4","3"});

    addTest("American Energy Revolution",
            {"13","28","35","33","51","27","12","36","34","14","21","29","31",
             "30","25","32","26","24","11","54","49","16","46","48","10","18",
             "4","50","42","5","23","47","20","43","37","39","41","45","2",
             "22","3","40","7","17","52","44","53","19","9","55","1","15","8"});

    addTest("Future of new America",
            {"51","14","35","26","13","50","33","4","12","31","20","32","29",
             "34","41","27","46","30","40","25","17","42","16","24","54","2",
             "43","21","7","18","52","49","5","47","45","10","36","38","44",
             "11","3","39","22","9","48","37","23","15","1","19","55","6","8"});

    addTest("Hillary clinton is the worst looser",
            {"4","12","36","42","33","20","29","35","16","22","47","34","14",
             "21","45","46","41","11","3","51","53","40","37","17","48","39",
             "5","28","10","18","54","44","25","43","49","24","30","32","8",
             "50","9","26","7","19","52","31","27","6","2","38","1","55"});

    addTest("no patience for injustice",
            {"11","22","16","7","15"});

    addTest("Global interests",
            {"10","16","27","42","21","11","25","22","47","41","48","54",
             "18","40","24","39","8"});

    addTest("pakistan afghanistan aid",
            {"3","22","42","29","41","16","40","39","17","4","1"});

    addTest("biggest plane wanted hour",
            {"52","1","19"});

    addTest("near architect box",
            {"54","23","18","24","39","43","51","4","9","17","11"});

    addTest("peaceful change",
            {"14","10","4","30","3","11","20","16","50","52","48","17","7",
             "2","22","29","13","53","41","46","32","21","39","25","33","54",
             "42","5","36","31","43","51","49","23","47","35","37","12","45","8"});

    //Run tests
    int passed = 0;
    int total  = (int)tests.size();

    cout << "  VSM Query Evaluation  (" << total << " queries)\n";

    for (auto& tc : tests) {
        // Use alpha=0.0 so we always have enough ranked results to compare.
        auto results = vsmQuery(tc.query,0.0);

        int topK = (int)tc.expected.size();
        set<string> got;
        for (int i = 0; i < topK && i < (int)results.size(); ++i)
            got.insert(to_string(results[i].docId));

        bool ok = (got == tc.expected);

        cout << (ok ? "[PASSED]" : "[FAILED]") << "  \"" << tc.query << "\"\n";
        cout << "  Total retrieved: " << results.size()
             << "  |  Checking top-" << topK << "\n";

        if (ok) {
            // Show top-5 ranked results on pass
            int show = min((int)results.size(), 5);
            for (int i = 0; i < show; ++i)
                cout << "    #" << i+1 << "  [doc " << results[i].docId << "]  "
                     << results[i].filename
                     << "   cosine=" << results[i].score << "\n";
        } else {
            // Diagnose mismatches on fail
            set<string> missing, extra;
            for (auto& x : tc.expected) if (!got.count(x))  missing.insert(x);
            for (auto& x : got)         if (!tc.expected.count(x)) extra.insert(x);

            if (!missing.empty()) {
                cout << "  Missing from top-" << topK << ": {";
                for (auto& x : missing) cout << x << " ";
                cout << "}\n";
            }
            if (!extra.empty()) {
                cout << "  Unexpected in top-" << topK << ": {";
                for (auto& x : extra)   cout << x << " ";
                cout << "}\n";
            }
        }
        cout << "\n";
        if (ok) ++passed;
    }

    cout << "========================================\n";
    cout << "  Result: " << passed << " / " << total << " queries passed\n";
    cout << "========================================\n";
}

//  Interactive search console

void searchConsole() {
    cout << "\n-----------------------------------------\n";
    cout << "   VSM Search Console  (type 'exit' to quit)\n";
    cout << "------------------------------------------\n";

    string query;
    while (true) {
        cout << "\nQuery> ";
        if (!getline(cin, query)) break;
        if (toLower(trim(query)) == "exit") break;
        if (trim(query).empty()) continue;

        auto results = vsmQuery(query, 0.005);
        if (results.empty()) {
            cout << "  No documents found above threshold (alpha=0.005).\n";
            continue;
        }
        cout << "  Found " << results.size() << " document(s):\n";
        for (auto& r : results)
            cout << "    [doc " << r.docId << "]  " << r.filename
                 << "   cosine=" << r.score << "\n";
    }
    cout << "Goodbye.\n";
}


int main() {

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

    auto loadJson = [](const string& path) -> json {
        ifstream f(path);
        if (!f.is_open()) { cerr << "Cannot open index file: " << path << "\n"; exit(1); }
        json j; f >> j; return j;
    };
    
    json jTfidf  = loadJson("tfidf_index.json");
    json jDf     = loadJson("df_index.json");
    json jNorms  = loadJson("doc_norms.json");
    json jDocMap = loadJson("doc_map.json");
    cout << "done.\n";

    // Populate tfidfIndex
    for (auto it = jTfidf.begin(); it != jTfidf.end(); ++it) {
        string term = it.key();
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt)
            tfidfIndex[term][jt.key()] = jt.value().get<double>();
    }

    // Populate dfIndex
    for (auto it = jDf.begin(); it != jDf.end(); ++it)
        dfIndex[it.key()] = it.value().get<int>();

    // Populate docNorms
    for (auto it = jNorms.begin(); it != jNorms.end(); ++it)
        docNorms[it.key()] = it.value().get<double>();

    // Populate docMap
    for (auto it = jDocMap.begin(); it != jDocMap.end(); ++it)
        docMap[it.key()] = it.value().get<string>();

    N = (int)docMap.size();
    cout << "Corpus: N=" << N << " documents,  "
         << tfidfIndex.size() << " unique terms.\n\n";

    //Run automated evaluation
    testQueries();

    //Uncomment for Interactive search console
   // searchConsole();

    return 0;
}