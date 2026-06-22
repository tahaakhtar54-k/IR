# IR Assignment 2 — Vector Space Model (VSM) Search Engine

A full Information Retrieval pipeline built over a corpus of 56 Trump speeches. It indexes documents using TF-IDF weights, then answers free-text queries via cosine similarity in the Vector Space Model.

---

## Project Structure

```
IR Assignment 2/
├── preprocessing.cpp        # C++ indexer — builds all JSON index files
├── preprocessing.exe        # Compiled Windows binary
├── queryprocessing.cpp      # C++ query engine + automated evaluation harness
├── queryprocessing.exe      # Compiled Windows binary
├── gui.py                   # Python/Tkinter GUI front-end for interactive search
├── include/
│   └── json.hpp             # nlohmann/json single-header library
├── Trump Speechs/           # Corpus: speech_0.txt … speech_55.txt (56 documents)
├── Stopword-List.txt        # 26 stop words (a, is, the, of, …)
├── Query List VSM.txt       # Evaluation queries with expected result sets
├── tf_index.json            # Raw term-frequency index  { term → { docId → tf } }
├── df_index.json            # Document-frequency index  { term → df }
├── tfidf_index.json         # TF-IDF index              { term → { docId → score } }
├── doc_map.json             # Document ID → filename map
└── doc_norms.json           # L2 norm of each document's TF-IDF vector
```

---

## How It Works

### 1. Preprocessing (`preprocessing.cpp`)

Reads every `.txt` file in `Trump Speechs/`, applies the full pipeline, and writes five JSON index files.

**Pipeline per token:**
1. **Tokenise** — extract alphabetic runs only (punctuation and digits discarded).
2. **Case-fold** — convert to lowercase.
3. **Stem** — apply a Porter Stemmer (Steps 1–5b).
4. **Stop-word filter** — discard tokens whose stemmed form matches any stemmed stop word from `Stopword-List.txt`.

**Index construction:**
- `tf_index.json` — raw term frequency: how many times each term appears in each document.
- `df_index.json` — document frequency: how many documents contain each term.
- `tfidf_index.json` — TF-IDF score per (term, document) pair using the formula:

  ```
  tfidf(t, d) = tf(t, d) × log(N / df(t))
  ```

  where N = 56 (total documents).
- `doc_norms.json` — the Euclidean (L2) norm of each document's TF-IDF vector, pre-computed for fast cosine similarity at query time.
- `doc_map.json` — maps integer document IDs (0–55) to filenames.

### 2. Query Processing (`queryprocessing.cpp`)

Loads the pre-built JSON indices and answers queries using the Vector Space Model.

**Query pipeline:**
1. Tokenise, lowercase, and stem the query using the same Porter Stemmer.
2. Remove stop words.
3. Build a query TF-IDF vector (using the same IDF values from the corpus).
4. Compute the **cosine similarity** between the query vector and every document vector:

   ```
   cosine(q, d) = (q · d) / (||q|| × ||d||)
   ```

5. Return documents sorted by descending cosine score. Documents below threshold `α` (default `0.005`) are suppressed.

The binary also includes a **self-evaluation harness** (`testQueries`) that runs 17 labelled queries and checks whether the top-K ranked results exactly match the expected document sets.

### 3. GUI (`gui.py`)

A Tkinter desktop application providing an interactive search interface. It reimplements the same Porter Stemmer and VSM query logic in Python and loads the same JSON index files.

**Features:**
- Free-text search box with a configurable α threshold slider.
- Results table showing rank, document ID, filename, and cosine score.
- Query history panel (last 6 queries, clickable to re-run).

---

## Getting Started

### Prerequisites

| Component | Requirement |
|-----------|-------------|
| Preprocessing / Query engine | Windows (pre-compiled `.exe` provided) or a C++17 compiler |
| GUI | Python 3.10+ with Tkinter (usually bundled with CPython) |
| C++ dependency | `include/json.hpp` (already included) |

### Running the Pre-built Binaries (Windows)

Both executables must be run from the `IR Assignment 2/` directory so they can find the adjacent files.

```cmd
cd "IR Assignment 2"

:: Step 1 — build the index (only needed once, or after corpus changes)
preprocessing.exe

:: Step 2 — run query evaluation
queryprocessing.exe
```

`preprocessing.exe` reads `Trump Speechs/` and `Stopword-List.txt`, then writes the five JSON index files.  
`queryprocessing.exe` reads the JSON indices, runs the 17 evaluation queries, and prints pass/fail results.

### Running the GUI

```cmd
cd "IR Assignment 2"
python gui.py
```

The GUI loads the JSON indices automatically (looks in the same directory as `gui.py`).

### Recompiling from Source (optional)

```bash
# GCC / MinGW example
g++ -std=c++17 -O2 -o preprocessing  preprocessing.cpp
g++ -std=c++17 -O2 -o queryprocessing queryprocessing.cpp
```

The only external dependency is `include/json.hpp` (already present).

---

## Evaluation

`queryprocessing.cpp` runs 17 test queries automatically on startup. For each query it:
- Retrieves the full ranked list.
- Takes the top-K results (where K = size of the expected set).
- Compares the set of document IDs against the ground-truth set.
- Prints `[PASSED]` / `[FAILED]` and, on failure, which documents were missing or unexpected.

Example output:
```
[PASSED]  "pakistan afghanistan"
  Total retrieved: 6  |  Checking top-6
    #1  [doc 3]  speech_3.txt   cosine=0.412
    ...
========================================
  Result: 17 / 17 queries passed
========================================
```

---

## Key Design Decisions

- **Stemmed stop words** — stop words are themselves stemmed before being added to the filter set, ensuring consistent matching (e.g. "running" is filtered because its stem matches the stemmed form of a stop word, not just its raw form).
- **Pre-computed document norms** — `doc_norms.json` avoids recomputing L2 norms at query time, keeping latency low.
- **Threshold α** — queries with very broad terms can return dozens of weakly matching documents; the α cutoff (default 0.005) keeps results meaningful. Set α = 0.0 in the evaluation harness to retrieve all results for fair top-K comparison.
- **No stemming of query OOV terms** — if a query term does not appear in `df_index` after stemming, it is silently skipped (unknown vocabulary contributes nothing to the VSM score).
