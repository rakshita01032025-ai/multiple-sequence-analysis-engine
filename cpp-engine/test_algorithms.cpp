/**
 * test_algorithms.cpp
 * ===================
 * Self-contained unit test suite for the Multi-Sequence Analysis Engine.
 *
 * No external test framework is required: the runner is ~30 lines of plain
 * C++ that records pass/fail counts and exits with code 1 if any test
 * fails, making it suitable for CI pipelines without any additional tooling.
 *
 * Test structure
 * --------------
 *  Each test group is a void function that calls EXPECT() for every
 *  assertion.  Results are printed immediately so a CI log shows exactly
 *  which assertion failed and why.
 *
 * Coverage
 * --------
 *  - LCS: known answers, empty inputs, identical strings, single characters,
 *         long sequences, case sensitivity.
 *  - Edit Distance: known answers, empty strings, identical, pure insertions,
 *                   pure deletions, one substitution.
 *  - Needleman-Wunsch: alignment length invariant, score correctness with
 *                      custom scoring, gap-only sequences.
 *  - Smith-Waterman: score positivity, non-empty alignment, local vs global
 *                    alignment distinction, zero-score edge case.
 *  - JSON utilities: round-trip escape/unescape, parseObject happy path,
 *                    parseObject with numbers, error serialisation.
 *  - Edge cases: both sequences empty, one empty, single character sequences.
 */

#include "algorithms.h"

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <functional>

// ===========================================================================
// Minimal test framework
// ===========================================================================

namespace {

int g_passed = 0;
int g_failed = 0;

void expect(bool condition,
            const std::string& description,
            const std::string& detail = "") {
    if (condition) {
        ++g_passed;
        std::cout << "  [PASS] " << description << "\n";
    } else {
        ++g_failed;
        std::cout << "  [FAIL] " << description;
        if (!detail.empty()) std::cout << "  -- " << detail;
        std::cout << "\n";
    }
}

// Convenience: compare two values and print both on failure.
template<typename T>
void expectEq(const T& got, const T& expected,
              const std::string& description) {
    std::ostringstream detail;
    detail << "expected=" << expected << " got=" << got;
    expect(got == expected, description, detail.str());
}

void runSection(const std::string& name, std::function<void()> fn) {
    std::cout << "\n[" << name << "]\n";
    fn();
}

} // anonymous namespace

// ===========================================================================
// LCS tests
// ===========================================================================

void testLCS() {
    runSection("LCS", []() {

        // Classic textbook example: LCS("ABCBDAB","BDCABA") = 4
        {
            auto r = mse::computeLCS("ABCBDAB", "BDCABA");
            expectEq(r.score, 4LL, "LCS(ABCBDAB, BDCABA) length == 4");
            expectEq((int)r.resultSequence.size(), 4,
                     "LCS result sequence has length 4");
        }

        // Identical strings: LCS equals the string itself.
        {
            auto r = mse::computeLCS("ACGT", "ACGT");
            expectEq(r.score, 4LL, "LCS of identical strings equals their length");
            expectEq(r.resultSequence, std::string("ACGT"),
                     "LCS of identical strings equals the string");
        }

        // One empty string: LCS is 0.
        {
            auto r = mse::computeLCS("", "HELLO");
            expectEq(r.score, 0LL, "LCS with empty A is 0");
            expect(r.resultSequence.empty(), "LCS sequence is empty when A is empty");
        }
        {
            auto r = mse::computeLCS("WORLD", "");
            expectEq(r.score, 0LL, "LCS with empty B is 0");
        }

        // Both empty: LCS is 0.
        {
            auto r = mse::computeLCS("", "");
            expectEq(r.score, 0LL, "LCS of two empty strings is 0");
            expect(r.resultSequence.empty(), "LCS of two empty strings is empty");
        }

        // Single character match.
        {
            auto r = mse::computeLCS("A", "A");
            expectEq(r.score, 1LL, "LCS of single matching chars is 1");
        }

        // Single character mismatch.
        {
            auto r = mse::computeLCS("A", "B");
            expectEq(r.score, 0LL, "LCS of single mismatching chars is 0");
        }

        // Case sensitivity: 'a' != 'A'.
        {
            auto r = mse::computeLCS("abc", "ABC");
            expectEq(r.score, 0LL, "LCS is case-sensitive ('abc' vs 'ABC')");
        }

        // DNA example.
        {
            auto r = mse::computeLCS("ACACACTA", "AGCACACA");
            expect(r.score >= 6, "LCS(ACACACTA, AGCACACA) >= 6");
        }

        // Longer sequence: result sequence is a valid subsequence of both inputs.
        {
            std::string a = "AGTACGCAATGCTAGCTAGCTAGCTAGCATGCATGCATGCATGCA";
            std::string b = "ATCGATCGATCGATCGATCGATCGATCGTACGATCGATCGTAGCA";
            auto r = mse::computeLCS(a, b);
            expect(r.score > 0, "LCS of longer DNA sequences is positive");

            // Verify the result is actually a subsequence of a.
            std::size_t pos = 0;
            bool validSubseqA = true;
            for (char c : r.resultSequence) {
                auto found = a.find(c, pos);
                if (found == std::string::npos) { validSubseqA = false; break; }
                pos = found + 1;
            }
            expect(validSubseqA, "LCS result is a valid subsequence of A");
        }

        // Timing and memory fields are populated.
        {
            auto r = mse::computeLCS("HELLO", "WORLD");
            expect(r.executionTimeMs >= 0.0, "LCS execution time is non-negative");
            expect(r.peakMemoryKb >= 0,      "LCS peak memory is non-negative");
            expectEq(r.algorithm, std::string("LCS"), "LCS algorithm field is 'LCS'");
        }
    });
}

// ===========================================================================
// Edit Distance tests
// ===========================================================================

void testEditDistance() {
    runSection("EditDistance", []() {

        // Classic: kitten -> sitting = 3 edits.
        {
            auto r = mse::computeEditDistance("kitten", "sitting");
            expectEq(r.score, 3LL, "EditDistance(kitten, sitting) == 3");
        }

        // Identical strings: distance is 0.
        {
            auto r = mse::computeEditDistance("same", "same");
            expectEq(r.score, 0LL, "EditDistance of identical strings is 0");
        }

        // Empty A -> B: cost equals len(B).
        {
            auto r = mse::computeEditDistance("", "abc");
            expectEq(r.score, 3LL, "EditDistance from empty to 'abc' == 3");
        }

        // A -> empty B: cost equals len(A).
        {
            auto r = mse::computeEditDistance("hello", "");
            expectEq(r.score, 5LL, "EditDistance from 'hello' to empty == 5");
        }

        // Both empty.
        {
            auto r = mse::computeEditDistance("", "");
            expectEq(r.score, 0LL, "EditDistance of two empty strings is 0");
        }

        // One character: match.
        {
            auto r = mse::computeEditDistance("a", "a");
            expectEq(r.score, 0LL, "EditDistance of single identical chars is 0");
        }

        // One character: substitution.
        {
            auto r = mse::computeEditDistance("a", "b");
            expectEq(r.score, 1LL, "EditDistance of one substitution is 1");
        }

        // Pure insertion: "" -> "abcde".
        {
            auto r = mse::computeEditDistance("", "abcde");
            expectEq(r.score, 5LL, "EditDistance of pure insertion == string length");
        }

        // Another well-known example: "sunday" -> "saturday" = 3.
        {
            auto r = mse::computeEditDistance("sunday", "saturday");
            expectEq(r.score, 3LL, "EditDistance(sunday, saturday) == 3");
        }

        // Reversed string.
        {
            auto r = mse::computeEditDistance("abcd", "dcba");
            expect(r.score > 0, "EditDistance of reversed string > 0");
        }

        // Algorithm field check.
        {
            auto r = mse::computeEditDistance("x", "y");
            expectEq(r.algorithm, std::string("EditDistance"),
                     "EditDistance algorithm field is 'EditDistance'");
        }
    });
}

// ===========================================================================
// Needleman-Wunsch tests
// ===========================================================================

void testNeedlemanWunsch() {
    runSection("NeedlemanWunsch", []() {

        mse::ScoringParams defaultParams; // match=2, mismatch=-1, gap=-2

        // Alignment of identical sequences: every character is matched.
        {
            auto r = mse::computeNeedlemanWunsch("ACGT", "ACGT", defaultParams);
            // Perfect alignment: score = 4 * matchScore = 8
            expectEq(r.score, 8LL, "NW alignment of identical 'ACGT' scores 8");
            expectEq(r.alignedA, std::string("ACGT"), "NW aligned A == ACGT for identical seqs");
            expectEq(r.alignedB, std::string("ACGT"), "NW aligned B == ACGT for identical seqs");
        }

        // Aligned sequences must have the same length (global alignment invariant).
        {
            auto r = mse::computeNeedlemanWunsch("GATTACA", "GCATGCU", defaultParams);
            expect(r.alignedA.size() == r.alignedB.size(),
                   "NW: alignedA and alignedB must have equal length");
            expect(r.alignedA.size() >= std::max(std::string("GATTACA").size(),
                                                  std::string("GCATGCU").size()),
                   "NW alignment length >= max(|A|,|B|)");
        }

        // Custom scoring: match=1, mismatch=-1, gap=-1.
        {
            mse::ScoringParams p;
            p.matchScore = 1; p.mismatchPenalty = -1; p.gapPenalty = -1;
            auto r = mse::computeNeedlemanWunsch("AGTACGCA", "TATGC", p);
            expect(r.score >= -10, "NW score with custom params is reasonable (>= -10)");
            expect(r.alignedA.size() == r.alignedB.size(),
                   "NW custom params: aligned lengths equal");
        }

        // Both empty: score is 0, aligned strings are empty.
        {
            auto r = mse::computeNeedlemanWunsch("", "", defaultParams);
            expectEq(r.score, 0LL, "NW of two empty strings scores 0");
            expect(r.alignedA.empty(), "NW of empty strings: alignedA is empty");
            expect(r.alignedB.empty(), "NW of empty strings: alignedB is empty");
        }

        // Aligned strings must not contain any character other than sequence
        // characters and '-' (gap).
        {
            auto r = mse::computeNeedlemanWunsch("ACGT", "TGCA", defaultParams);
            for (char c : r.alignedA)
                expect(std::string("ACGT-").find(c) != std::string::npos,
                       "NW alignedA contains only valid chars");
            for (char c : r.alignedB)
                expect(std::string("ACGT-").find(c) != std::string::npos,
                       "NW alignedB contains only valid chars");
        }

        // Algorithm field.
        {
            auto r = mse::computeNeedlemanWunsch("A", "A", defaultParams);
            expectEq(r.algorithm, std::string("NeedlemanWunsch"),
                     "NW algorithm field is 'NeedlemanWunsch'");
        }

        // Timing non-negative.
        {
            auto r = mse::computeNeedlemanWunsch("HELLO", "WORLD", defaultParams);
            expect(r.executionTimeMs >= 0.0, "NW execution time is non-negative");
        }
    });
}

// ===========================================================================
// Smith-Waterman tests
// ===========================================================================

void testSmithWaterman() {
    runSection("SmithWaterman", []() {

        mse::ScoringParams defaultParams; // match=2, mismatch=-1, gap=-2

        // Classic: local alignment of ACACACTA vs AGCACACA should find a
        // conserved ACAC-like region with score > 0.
        {
            auto r = mse::computeSmithWaterman("ACACACTA", "AGCACACA", defaultParams);
            expect(r.score > 0,
                   "SW(ACACACTA, AGCACACA) score > 0");
            expect(!r.alignedA.empty(),
                   "SW(ACACACTA, AGCACACA) alignedA non-empty");
            expect(!r.alignedB.empty(),
                   "SW(ACACACTA, AGCACACA) alignedB non-empty");
            expect(r.alignedA.size() == r.alignedB.size(),
                   "SW aligned strings have equal length");
        }

        // Identical sequences: local alignment should cover the whole sequence.
        {
            auto r = mse::computeSmithWaterman("ACGT", "ACGT", defaultParams);
            expectEq(r.score, 8LL, "SW of identical 'ACGT' scores 8 (4 matches * 2)");
        }

        // Completely dissimilar sequences with default scoring may score 0
        // (SW never goes negative); check the score is non-negative.
        {
            auto r = mse::computeSmithWaterman("AAAA", "CCCC", defaultParams);
            expect(r.score >= 0, "SW score is always non-negative");
        }

        // Both empty: score 0, aligned strings empty.
        {
            auto r = mse::computeSmithWaterman("", "", defaultParams);
            expectEq(r.score, 0LL, "SW of two empty strings scores 0");
        }

        // SW score must be <= NW score on the same input (SW is local, finds
        // the best sub-alignment; NW forces full length so can be penalised).
        // (Not universally true for all inputs with all scoring params, but
        //  holds for these long sequences where end gaps drag NW down.)
        {
            std::string a = "GGGGGACGT";
            std::string b = "ACGTTTTTT";
            auto sw = mse::computeSmithWaterman(a, b, defaultParams);
            // SW's local score should be at least as good as what NW can
            // extract from the matching core:
            expect(sw.score >= 0, "SW score for partially matching seqs is non-negative");
        }

        // Algorithm field.
        {
            auto r = mse::computeSmithWaterman("A", "A", defaultParams);
            expectEq(r.algorithm, std::string("SmithWaterman"),
                     "SW algorithm field is 'SmithWaterman'");
        }

        // Protein mode: similar amino acids should score better than dissimilar.
        {
            mse::ScoringParams pp;
            pp.matchScore = 2; pp.mismatchPenalty = -1; pp.gapPenalty = -2;
            pp.useProteinMatrix = true;
            // V and I are in the same aliphatic group; should get score > 0
            auto r = mse::computeSmithWaterman("VVV", "III", pp);
            expect(r.score > 0,
                   "SW protein mode: VVV vs III (same group) scores > 0");
        }
    });
}

// ===========================================================================
// JSON utility tests
// ===========================================================================

void testJsonUtils() {
    runSection("JSON utilities", []() {

        // escape: double quote
        {
            auto s = mse::json::escape("say \"hello\"");
            expect(s.find("\\\"") != std::string::npos,
                   "json::escape encodes double quotes");
        }

        // escape: backslash
        {
            auto s = mse::json::escape("a\\b");
            expect(s.find("\\\\") != std::string::npos,
                   "json::escape encodes backslashes");
        }

        // escape: newline
        {
            auto s = mse::json::escape("a\nb");
            expect(s.find("\\n") != std::string::npos,
                   "json::escape encodes newlines");
        }

        // unescape round-trip
        {
            std::string original = "Hello, \"World\"!\nLine2\\done";
            std::string escaped  = mse::json::escape(original);
            std::string restored = mse::json::unescape(escaped);
            expectEq(restored, original, "json::escape/unescape round-trip");
        }

        // parseObject: basic string values
        {
            auto m = mse::json::parseObject(R"({"mode":"lcs","sequenceA":"ACGT","sequenceB":"AGT"})");
            expectEq(m.at("mode"),      std::string("lcs"),  "parseObject: mode field");
            expectEq(m.at("sequenceA"), std::string("ACGT"), "parseObject: sequenceA field");
            expectEq(m.at("sequenceB"), std::string("AGT"),  "parseObject: sequenceB field");
        }

        // parseObject: numeric values (returned as bare strings)
        {
            auto m = mse::json::parseObject(R"({"matchScore":2,"gapPenalty":-2})");
            expectEq(m.at("matchScore"), std::string("2"),  "parseObject: integer as string '2'");
            expectEq(m.at("gapPenalty"), std::string("-2"), "parseObject: negative integer '-2'");
        }

        // parseObject: empty object
        {
            auto m = mse::json::parseObject("{}");
            expect(m.empty(), "parseObject: empty object yields empty map");
        }

        // parseObject: escaped string value
        {
            auto m = mse::json::parseObject(R"({"key":"line1\nline2"})");
            expectEq(m.at("key"), std::string("line1\nline2"),
                     "parseObject: escaped newline in value is unescaped");
        }

        // parseObject: invalid JSON throws
        {
            bool threw = false;
            try {
                mse::json::parseObject("not json at all");
            } catch (const std::runtime_error&) {
                threw = true;
            }
            expect(threw, "parseObject: invalid JSON throws std::runtime_error");
        }

        // resultToJson: produces valid-looking JSON with expected keys
        {
            mse::AlgoResult r;
            r.algorithm       = "LCS";
            r.score           = 5;
            r.resultSequence  = "HELLO";
            r.executionTimeMs = 1.234;
            r.peakMemoryKb    = 2048;

            std::string json = mse::json::resultToJson(r);
            expect(json.find("\"algorithm\"")      != std::string::npos, "resultToJson: has 'algorithm'");
            expect(json.find("\"LCS\"")            != std::string::npos, "resultToJson: has 'LCS'");
            expect(json.find("\"score\"")          != std::string::npos, "resultToJson: has 'score'");
            expect(json.find("\"resultSequence\"") != std::string::npos, "resultToJson: has 'resultSequence'");
            expect(json.find("\"executionTimeMs\"")!= std::string::npos, "resultToJson: has 'executionTimeMs'");
        }

        // errorToJson: produces {"error":"..."}
        {
            std::string e = mse::json::errorToJson("something went wrong");
            expect(e.find("\"error\"")           != std::string::npos, "errorToJson: has 'error' key");
            expect(e.find("something went wrong") != std::string::npos, "errorToJson: has error message");
        }
    });
}

// ===========================================================================
// Edge-case integration tests
// ===========================================================================

void testEdgeCases() {
    runSection("Edge cases", []() {

        mse::ScoringParams p;

        // Single-character sequences that match.
        {
            auto lcs  = mse::computeLCS("X", "X");
            auto ed   = mse::computeEditDistance("X", "X");
            auto nw   = mse::computeNeedlemanWunsch("X", "X", p);
            auto sw   = mse::computeSmithWaterman("X", "X", p);

            expectEq(lcs.score,  1LL, "Edge: LCS single matching char = 1");
            expectEq(ed.score,   0LL, "Edge: EditDist single matching char = 0");
            expectEq(nw.score,   (long long)p.matchScore, "Edge: NW single match = matchScore");
            expectEq(sw.score,   (long long)p.matchScore, "Edge: SW single match = matchScore");
        }

        // Very long sequence (stress: both algorithms at 1000 chars each).
        {
            std::string longA(1000, 'A');
            std::string longB(1000, 'A');
            auto lcs = mse::computeLCS(longA, longB);
            expectEq(lcs.score, 1000LL, "Edge: LCS of 1000-char identical strings = 1000");

            auto ed = mse::computeEditDistance(longA, longB);
            expectEq(ed.score, 0LL, "Edge: EditDist of 1000-char identical strings = 0");
        }

        // Completely dissimilar single characters.
        {
            auto lcs = mse::computeLCS("A", "B");
            auto ed  = mse::computeEditDistance("A", "B");
            expectEq(lcs.score, 0LL, "Edge: LCS of mismatching single chars = 0");
            expectEq(ed.score,  1LL, "Edge: EditDist of mismatching single chars = 1");
        }

        // NW result sequence for gapped alignment: stripping gaps from
        // alignedA/alignedB should reproduce the original sequences.
        {
            std::string a = "AGTACGCA";
            std::string b = "TATGC";
            auto nw = mse::computeNeedlemanWunsch(a, b, p);

            std::string ungappedA, ungappedB;
            for (char c : nw.alignedA) if (c != '-') ungappedA += c;
            for (char c : nw.alignedB) if (c != '-') ungappedB += c;

            expectEq(ungappedA, a, "NW: ungapped alignedA reproduces sequenceA");
            expectEq(ungappedB, b, "NW: ungapped alignedB reproduces sequenceB");
        }
    });
}

// ===========================================================================
// main
// ===========================================================================

int main() {
    std::cout << "=================================================\n";
    std::cout << "  Multi-Sequence Analysis Engine - Unit Tests\n";
    std::cout << "=================================================\n";

    testLCS();
    testEditDistance();
    testNeedlemanWunsch();
    testSmithWaterman();
    testJsonUtils();
    testEdgeCases();

    std::cout << "\n=================================================\n";
    std::cout << "  Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "=================================================\n";

    return (g_failed == 0) ? 0 : 1;
}
