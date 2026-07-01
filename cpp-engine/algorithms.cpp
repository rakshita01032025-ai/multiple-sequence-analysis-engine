#include "algorithms.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace mse {

// ===========================================================================
// System helper
// ===========================================================================

long long getPeakMemoryKb() {
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) return 0;

    std::string line;
    while (std::getline(statusFile, line)) {
        if (line.compare(0, 6, "VmHWM:") == 0) {
            std::istringstream iss(line.substr(6));
            long long kb = 0;
            iss >> kb;
            return kb;
        }
    }
    return 0;
}

// ===========================================================================
// Internal helpers
// ===========================================================================

namespace {

// Simplified BLOSUM-like residue similarity groups.
// Members of the same group get a small positive score on mismatch; members
// in different groups receive the standard mismatchPenalty.
const std::unordered_map<char, const char*> kProteinGroups = {
    // Aliphatic / hydrophobic
    {'A', "AVILMC"}, {'V', "AVILMC"}, {'I', "AVILMC"},
    {'L', "AVILMC"}, {'M', "AVILMC"}, {'C', "AVILMC"},
    // Aromatic
    {'F', "FWY"},    {'W', "FWY"},    {'Y', "FWY"},
    // Acidic
    {'D', "DE"},     {'E', "DE"},
    // Basic
    {'K', "KRH"},    {'R', "KRH"},    {'H', "KRH"},
    // Small polar / amide
    {'S', "STNQ"},   {'T', "STNQ"},   {'N', "STNQ"}, {'Q', "STNQ"},
    // Special
    {'G', "G"},      {'P', "P"},
};

inline int scorePair(char x, char y, const ScoringParams& p) {
    if (x == y) return p.matchScore;

    if (p.useProteinMatrix) {
        char X = static_cast<char>(std::toupper(static_cast<unsigned char>(x)));
        char Y = static_cast<char>(std::toupper(static_cast<unsigned char>(y)));
        auto it = kProteinGroups.find(X);
        if (it != kProteinGroups.end()) {
            const char* grp = it->second;
            for (const char* c = grp; *c; ++c) {
                if (*c == Y) return 1; // conservative match within group
            }
        }
    }
    return p.mismatchPenalty;
}

} // anonymous namespace

// ===========================================================================
// 1. Longest Common Subsequence
// ===========================================================================

AlgoResult computeLCS(const std::string& a, const std::string& b) {
    auto t0 = std::chrono::high_resolution_clock::now();

    const std::size_t n = a.size();
    const std::size_t m = b.size();

    // dp[i][j] = length of LCS of a[0..i) and b[0..j)
    // Using a 2-D vector; for very long sequences the dominant allocation is
    // O(n*m) integers -- about 25 MB for n=m=2500 with int32, which is fine.
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (std::size_t i = 1; i <= n; ++i) {
        for (std::size_t j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Traceback: reconstruct the actual LCS string.
    std::string lcs;
    lcs.reserve(static_cast<std::size_t>(dp[n][m]));
    std::size_t i = n, j = m;
    while (i > 0 && j > 0) {
        if (a[i - 1] == b[j - 1]) {
            lcs.push_back(a[i - 1]);
            --i; --j;
        } else if (dp[i - 1][j] >= dp[i][j - 1]) {
            --i;
        } else {
            --j;
        }
    }
    std::reverse(lcs.begin(), lcs.end());

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    AlgoResult result;
    result.algorithm      = "LCS";
    result.score          = static_cast<long long>(dp[n][m]);
    result.resultSequence = std::move(lcs);
    result.executionTimeMs = ms;
    result.peakMemoryKb   = getPeakMemoryKb();
    return result;
}

// ===========================================================================
// 2. Edit Distance (Levenshtein)
// ===========================================================================

AlgoResult computeEditDistance(const std::string& a, const std::string& b) {
    auto t0 = std::chrono::high_resolution_clock::now();

    const std::size_t n = a.size();
    const std::size_t m = b.size();

    // dp[i][j] = minimum edits to transform a[0..i) into b[0..j)
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (std::size_t i = 0; i <= n; ++i) dp[i][0] = static_cast<int>(i);
    for (std::size_t j = 0; j <= m; ++j) dp[0][j] = static_cast<int>(j);

    for (std::size_t i = 1; i <= n; ++i) {
        for (std::size_t j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1]; // no cost
            } else {
                const int sub    = dp[i - 1][j - 1] + 1; // substitute
                const int del_op = dp[i - 1][j]     + 1; // delete from a
                const int ins_op = dp[i][j - 1]     + 1; // insert into a
                dp[i][j] = std::min({sub, del_op, ins_op});
            }
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    AlgoResult result;
    result.algorithm       = "EditDistance";
    result.score           = static_cast<long long>(dp[n][m]);
    result.executionTimeMs = ms;
    result.peakMemoryKb    = getPeakMemoryKb();
    return result;
}

// ===========================================================================
// 3. Needleman-Wunsch (global alignment)
// ===========================================================================

AlgoResult computeNeedlemanWunsch(const std::string& a,
                                  const std::string& b,
                                  const ScoringParams& params) {
    auto t0 = std::chrono::high_resolution_clock::now();

    const std::size_t n = a.size();
    const std::size_t m = b.size();

    // dp[i][j] = best global alignment score for a[0..i) vs b[0..j)
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    // Initialise first row and column with linear gap penalties.
    for (std::size_t i = 0; i <= n; ++i)
        dp[i][0] = static_cast<int>(i) * params.gapPenalty;
    for (std::size_t j = 0; j <= m; ++j)
        dp[0][j] = static_cast<int>(j) * params.gapPenalty;

    for (std::size_t i = 1; i <= n; ++i) {
        for (std::size_t j = 1; j <= m; ++j) {
            const int diag = dp[i - 1][j - 1] + scorePair(a[i-1], b[j-1], params);
            const int up   = dp[i - 1][j]     + params.gapPenalty; // gap in b
            const int left = dp[i][j - 1]     + params.gapPenalty; // gap in a
            dp[i][j] = std::max({diag, up, left});
        }
    }

    // Traceback from dp[n][m] to dp[0][0].
    std::string alignedA, alignedB;
    alignedA.reserve(n + m);
    alignedB.reserve(n + m);

    std::size_t i = n, j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 &&
            dp[i][j] == dp[i-1][j-1] + scorePair(a[i-1], b[j-1], params)) {
            alignedA.push_back(a[i - 1]);
            alignedB.push_back(b[j - 1]);
            --i; --j;
        } else if (i > 0 && dp[i][j] == dp[i-1][j] + params.gapPenalty) {
            alignedA.push_back(a[i - 1]);
            alignedB.push_back('-');
            --i;
        } else {
            alignedA.push_back('-');
            alignedB.push_back(b[j - 1]);
            --j;
        }
    }
    std::reverse(alignedA.begin(), alignedA.end());
    std::reverse(alignedB.begin(), alignedB.end());

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    AlgoResult result;
    result.algorithm       = "NeedlemanWunsch";
    result.score           = static_cast<long long>(dp[n][m]);
    result.alignedA        = std::move(alignedA);
    result.alignedB        = std::move(alignedB);
    result.executionTimeMs = ms;
    result.peakMemoryKb    = getPeakMemoryKb();
    return result;
}

// ===========================================================================
// 4. Smith-Waterman (local alignment)
// ===========================================================================

AlgoResult computeSmithWaterman(const std::string& a,
                                const std::string& b,
                                const ScoringParams& params) {
    auto t0 = std::chrono::high_resolution_clock::now();

    const std::size_t n = a.size();
    const std::size_t m = b.size();

    // dp[i][j] = best local alignment score ending at a[i-1] / b[j-1].
    // Crucially, scores never go below 0 (that's what makes it "local").
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    int bestScore  = 0;
    std::size_t bi = 0, bj = 0; // cell with highest score (traceback start)

    for (std::size_t i = 1; i <= n; ++i) {
        for (std::size_t j = 1; j <= m; ++j) {
            const int diag = dp[i-1][j-1] + scorePair(a[i-1], b[j-1], params);
            const int up   = dp[i-1][j]   + params.gapPenalty;
            const int left = dp[i][j-1]   + params.gapPenalty;
            dp[i][j] = std::max({0, diag, up, left});
            if (dp[i][j] > bestScore) {
                bestScore = dp[i][j];
                bi = i;
                bj = j;
            }
        }
    }

    // Traceback from the max-score cell until we hit a 0.
    std::string alignedA, alignedB;
    alignedA.reserve(std::min(n, m));
    alignedB.reserve(std::min(n, m));

    std::size_t i = bi, j = bj;
    while (i > 0 && j > 0 && dp[i][j] != 0) {
        const int cur  = dp[i][j];
        const int diag = dp[i-1][j-1] + scorePair(a[i-1], b[j-1], params);
        const int up   = dp[i-1][j]   + params.gapPenalty;

        if (cur == diag) {
            alignedA.push_back(a[i - 1]);
            alignedB.push_back(b[j - 1]);
            --i; --j;
        } else if (cur == up) {
            alignedA.push_back(a[i - 1]);
            alignedB.push_back('-');
            --i;
        } else {
            alignedA.push_back('-');
            alignedB.push_back(b[j - 1]);
            --j;
        }
    }
    std::reverse(alignedA.begin(), alignedA.end());
    std::reverse(alignedB.begin(), alignedB.end());

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    AlgoResult result;
    result.algorithm       = "SmithWaterman";
    result.score           = static_cast<long long>(bestScore);
    result.alignedA        = std::move(alignedA);
    result.alignedB        = std::move(alignedB);
    result.executionTimeMs = ms;
    result.peakMemoryKb    = getPeakMemoryKb();
    return result;
}

// ===========================================================================
// JSON utilities (namespace mse::json)
// ===========================================================================

namespace json {

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

std::string unescape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case '"':  out += '"';  ++i; break;
                case '\\': out += '\\'; ++i; break;
                case 'n':  out += '\n'; ++i; break;
                case 'r':  out += '\r'; ++i; break;
                case 't':  out += '\t'; ++i; break;
                default:   out += s[i];       break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

std::unordered_map<std::string, std::string> parseObject(const std::string& json_str) {
    std::unordered_map<std::string, std::string> result;
    std::size_t i = 0;
    const std::size_t n = json_str.size();

    auto skipWs = [&]() {
        while (i < n && std::isspace(static_cast<unsigned char>(json_str[i]))) ++i;
    };

    skipWs();
    if (i >= n || json_str[i] != '{')
        throw std::runtime_error("JSON parse error: expected '{'");
    ++i;

    while (true) {
        skipWs();
        if (i >= n) throw std::runtime_error("JSON parse error: unexpected end");
        if (json_str[i] == '}') { ++i; break; }

        // --- Parse key ---
        if (json_str[i] != '"')
            throw std::runtime_error("JSON parse error: expected key string");
        ++i;
        std::string key;
        while (i < n && json_str[i] != '"') {
            if (json_str[i] == '\\' && i + 1 < n) {
                key += json_str[i]; key += json_str[i + 1]; i += 2;
            } else {
                key += json_str[i++];
            }
        }
        if (i >= n) throw std::runtime_error("JSON parse error: unterminated key");
        key = unescape(key);
        ++i; // closing '"'

        skipWs();
        if (i >= n || json_str[i] != ':')
            throw std::runtime_error("JSON parse error: expected ':'");
        ++i;
        skipWs();

        // --- Parse value (string or bare token) ---
        std::string value;
        if (i < n && json_str[i] == '"') {
            ++i;
            while (i < n && json_str[i] != '"') {
                if (json_str[i] == '\\' && i + 1 < n) {
                    value += json_str[i]; value += json_str[i + 1]; i += 2;
                } else {
                    value += json_str[i++];
                }
            }
            if (i >= n) throw std::runtime_error("JSON parse error: unterminated value string");
            value = unescape(value);
            ++i; // closing '"'
        } else {
            // bare number / bool / null
            while (i < n && json_str[i] != ',' && json_str[i] != '}') {
                value += json_str[i++];
            }
            // strip trailing whitespace
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
                value.pop_back();
        }

        result[key] = value;

        skipWs();
        if (i < n && json_str[i] == ',') { ++i; continue; }
        skipWs();
        if (i < n && json_str[i] == '}') { ++i; break; }
    }
    return result;
}

std::string resultToJson(const AlgoResult& r) {
    std::ostringstream out;
    out << "{"
        << "\"algorithm\":"       << "\"" << escape(r.algorithm)       << "\","
        << "\"score\":"           << r.score                            << ","
        << "\"alignedA\":"        << "\"" << escape(r.alignedA)        << "\","
        << "\"alignedB\":"        << "\"" << escape(r.alignedB)        << "\","
        << "\"resultSequence\":"  << "\"" << escape(r.resultSequence)  << "\","
        << "\"executionTimeMs\":" << r.executionTimeMs                  << ","
        << "\"peakMemoryKb\":"    << r.peakMemoryKb
        << "}";
    return out.str();
}

std::string errorToJson(const std::string& message) {
    std::ostringstream out;
    out << "{\"error\":\"" << escape(message) << "\"}";
    return out.str();
}

} // namespace json
}//namespace mse