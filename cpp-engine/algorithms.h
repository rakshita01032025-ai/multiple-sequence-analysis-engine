#ifndef ALGORITHMS_H
#define ALGORITHMS_H

/**
 * algorithms.h
 * ============
 * Public interface for the Multi-Sequence Analysis Engine C++ core.
 *
 * Four algorithms are exposed:
 *   - Longest Common Subsequence (LCS)
 *   - Edit Distance (Levenshtein)
 *   - Needleman-Wunsch  (global alignment)
 *   - Smith-Waterman    (local  alignment)
 *
 * Each function accepts two character sequences and returns an AlgoResult
 * struct that carries every piece of data the Node.js layer needs to
 * persist and display: the numeric score, aligned strings (where applicable),
 * the recovered subsequence (LCS), wall-clock execution time, and the
 * process peak-RSS memory measured from /proc/self/status.
 *
 * Design notes
 * ------------
 *  - Zero external dependencies: only the C++17 standard library is used.
 *  - All heap allocation is proportional to O(n*m); there is no global state.
 *  - Scoring parameters are encapsulated in ScoringParams so callers never
 *    need to reach into the alignment internals.
 *  - The main.cpp entry point uses the json_util helpers declared here to
 *    read/write a flat JSON object on stdin/stdout; the same helpers are
 *    available to any future tool that links against this code.
 */

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace mse {

// ---------------------------------------------------------------------------
// Scoring configuration (Needleman-Wunsch & Smith-Waterman)
// ---------------------------------------------------------------------------

struct ScoringParams {
    int matchScore     =  2;   // reward for identical characters
    int mismatchPenalty= -1;   // penalty for substitution
    int gapPenalty     = -2;   // linear gap penalty (applied per gap character)

    // When true, biochemically similar amino-acid pairs receive a small bonus
    // (partial BLOSUM-like heuristic; keeps the engine dependency-free).
    bool useProteinMatrix = false;
};

// ---------------------------------------------------------------------------
// Unified result type returned by every algorithm
// ---------------------------------------------------------------------------

struct AlgoResult {
    std::string algorithm;       // "LCS" | "EditDistance" | "NeedlemanWunsch" | "SmithWaterman"
    long long   score      = 0;  // LCS length / edit distance / alignment score
    std::string alignedA;        // gapped alignment of sequence A (alignment algorithms only)
    std::string alignedB;        // gapped alignment of sequence B (alignment algorithms only)
    std::string resultSequence;  // reconstructed LCS string (LCS only)
    double      executionTimeMs = 0.0;
    long long   peakMemoryKb    = 0;
};

// ---------------------------------------------------------------------------
// Algorithm entry points
// ---------------------------------------------------------------------------

/**
 * Longest Common Subsequence.
 * O(n*m) time and space.  Backtracks to produce the actual subsequence.
 */
AlgoResult computeLCS(const std::string& a, const std::string& b);

/**
 * Levenshtein Edit Distance.
 * O(n*m) DP; unit costs for insert, delete, substitute.
 */
AlgoResult computeEditDistance(const std::string& a, const std::string& b);

/**
 * Needleman-Wunsch global alignment.
 * Returns the optimal global alignment score and the two gapped strings.
 */
AlgoResult computeNeedlemanWunsch(const std::string& a,
                                  const std::string& b,
                                  const ScoringParams& params);

/**
 * Smith-Waterman local alignment.
 * Returns the highest local alignment score and the best local aligned pair.
 */
AlgoResult computeSmithWaterman(const std::string& a,
                                const std::string& b,
                                const ScoringParams& params);

// ---------------------------------------------------------------------------
// System helpers
// ---------------------------------------------------------------------------

/**
 * Returns the peak resident set size of the current process in kilobytes.
 * Reads /proc/self/status (VmHWM field) on Linux; returns 0 on other OSes.
 */
long long getPeakMemoryKb();

// ---------------------------------------------------------------------------
// Minimal dependency-free JSON utilities (flat object only)
// Used by main.cpp to communicate with the Node.js bridge over stdin/stdout.
// ---------------------------------------------------------------------------

namespace json {

/**
 * Escapes a raw C++ string for safe embedding inside a JSON string literal
 * (handles backslashes, double quotes, control characters).
 */
std::string escape(const std::string& s);

/**
 * Reverses JSON string escaping (handles \n \r \t \\ \").
 */
std::string unescape(const std::string& s);

/**
 * Parses a flat JSON object whose values are strings or bare numbers/booleans.
 * Throws std::runtime_error for structurally invalid input.
 * This intentionally does NOT support nested objects or arrays; the protocol
 * between Node.js and the engine is defined to be a flat object on purpose.
 */
std::unordered_map<std::string, std::string> parseObject(const std::string& json);

/**
 * Serialises an AlgoResult to a single-line JSON object string.
 * Never throws.
 */
std::string resultToJson(const AlgoResult& r);

/**
 * Serialises an error message to a {"error":"..."} JSON object.
 */
std::string errorToJson(const std::string& message);

} // namespace json
} // namespace mse

#endif // ALGORITHMS_H
