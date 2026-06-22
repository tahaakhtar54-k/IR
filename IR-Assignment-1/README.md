# IR Assignment 1 — Boolean Information Retrieval System

A full Information Retrieval system built over a corpus of 56 Trump speeches. It supports Boolean queries, proximity search, and a graphical user interface — implemented across C++ (indexing & query processing) and Python (GUI).

---

## Project Structure

```
IR Assignment #1/
├── preprocessing.cpp        # Builds inverted index & positional index from corpus
├── preprocessing.exe        # Compiled binary (Windows)
├── queryprocessing.cpp      # Processes Boolean and proximity queries + test suite
├── queryprocessing.exe      # Compiled binary (Windows)
├── gui.py                   # Tkinter-based graphical search interface
├── inverted_index.json      # Generated: term → [doc IDs]
├── positional_index.json    # Generated: term → {doc ID → [positions]}
├── doc_map.json             # Generated: doc ID → filename
├── Stopword-List.txt        # Stop words used during preprocessing
├── Querry List.txt          # Sample queries with expected results
├── include/
│   └── json.hpp             # nlohmann/json single-header library
└── Trump Speechs/
    ├── speech_0.txt
    ├── speech_1.txt
    └── ... (56 speeches total)
```

---

## Features

### Preprocessing (`preprocessing.cpp`)
- Reads all `.txt` files from the `Trump Speechs/` directory
- **Tokenizes** text by extracting alphabetic words
- **Lowercases** all tokens
- **Removes stop words** (loaded from `Stopword-List.txt`)
- **Stems** every token using a built-in **Porter Stemmer**
- Builds and serializes to JSON:
  - **Inverted Index** — maps each term to the sorted list of document IDs it appears in
  - **Positional Index** — maps each term to per-document lists of word positions
  - **Document Map** — maps numeric document IDs to filenames

### Query Processing (`queryprocessing.cpp`)
Loads the pre-built JSON indexes and supports the following query types:

| Query Type | Example |
|---|---|
| Single term | `running` |
| Boolean AND | `actions AND wanted` |
| Boolean OR | `united OR plane` |
| Boolean NOT | `not hammer` |
| Three-term Boolean | `pakistan OR afganistan OR aid` |
| AND NOT / OR NOT | `t1 AND NOT t2` |
| Parenthesized expressions | `biggest AND ( near OR box )` |
| NOT of parenthesized expression | `NOT (united AND plane)` |
| Proximity search | `after years /1` (within 1 word apart) |

All query terms are lowercased and stemmed before lookup, matching the preprocessing pipeline.

The binary also includes a built-in **test suite** (`testQueries()`) that validates 13 queries against expected document sets and reports a pass/fail count.

### GUI (`gui.py`)
A Python Tkinter desktop application that provides:
- A search input field for entering any supported query
- Real-time query execution against the JSON indexes
- Display of matching speech filenames with their document IDs
- Speech preview — clicking a result opens the full text of that speech
- The same Porter Stemmer and query logic ported to Python for consistency

---

## Getting Started

### Prerequisites
- **C++ compiler** (g++ or MSVC) supporting C++11 or later
- **Python 3.x** with `tkinter` (included in standard library)

### Step 1 — Build the Indexes

Compile and run `preprocessing.cpp` from the project root:

```bash
g++ -std=c++11 -O2 -o preprocessing preprocessing.cpp
./preprocessing
```

This reads all speeches from `Trump Speechs/` and outputs:
- `inverted_index.json`
- `positional_index.json`
- `doc_map.json`

> On Windows, the pre-built `preprocessing.exe` can be used directly.

### Step 2 — Run Query Tests

Compile and run `queryprocessing.cpp`:

```bash
g++ -std=c++11 -O2 -o queryprocessing queryprocessing.cpp
./queryprocessing
```

This loads the JSON indexes, runs the built-in test suite, and prints PASSED/FAILED for each query along with the matched documents.

To enable the interactive search console instead, uncomment the `InteractiveSearch()` call in `main()` before compiling.

### Step 3 — Launch the GUI

```bash
python gui.py
```

The GUI window will open. Type any supported query into the search box and press Enter or click Search.

---

## Query Syntax Reference

```
# Single term (stemmed automatically)
running

# Boolean operators (case-insensitive)
actions AND wanted
united OR plane
not hammer

# Three-term
pakistan OR afganistan OR aid

# Parenthesized sub-expressions
biggest AND ( near OR box )
NOT (united AND plane)

# Proximity search — terms within k words of each other
after years /1
develop solutions /1
keep out /2
```

---

## Implementation Notes

- **Porter Stemmer** is implemented from scratch in both C++ and Python — no external NLP library is used.
- **Stop word filtering** is applied after stemming, so stemmed variants of stop words are also excluded.
- **Posting list merging** uses linear-time merge algorithms (similar to merge sort merge) for AND (intersection) and OR (union).
- **NOT queries** are computed by finding the complement of a posting list against all known document IDs.
- **Proximity queries** use the positional index and check whether any pair of positions between two terms falls within the specified distance `k`.
- The `nlohmann/json` single-header library (`include/json.hpp`) handles all JSON serialization and deserialization in C++.

---

## Corpus

The document collection consists of **56 speeches** (`speech_0.txt` through `speech_55.txt`) sourced from Trump speeches (circa 2020). Each file contains the full plain text of one speech.
