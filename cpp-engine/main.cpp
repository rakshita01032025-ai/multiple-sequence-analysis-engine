
#include "algorithms.h"

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

std::string readStdin() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
}

/**
 * Reads an integer field from the parsed request map, returning the default
 * value if the field is absent or empty.
 */
int optInt(const std::unordered_map<std::string, std::string>& req,
           const std::string& key, int defaultValue) {
    auto it = req.find(key);
    if (it == req.end() || it->second.empty()) return defaultValue;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultValue;
    }
}

mse::ScoringParams buildScoringParams(const std::unordered_map<std::string, std::string>& req) {
    mse::ScoringParams p;
    p.matchScore      = optInt(req, "matchScore",      2);
    p.mismatchPenalty = optInt(req, "mismatchPenalty", -1);
    p.gapPenalty      = optInt(req, "gapPenalty",      -2);

    auto it = req.find("sequenceType");
    if (it != req.end() && it->second == "protein") {
        p.useProteinMatrix = true;
    }
    return p;
}

void respond(const mse::AlgoResult& result) {
    std::cout << mse::json::resultToJson(result) << std::endl;
}

void respondError(const std::string& message) {
    std::cout << mse::json::errorToJson(message) << std::endl;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    // Prevent any output buffering so the Node.js pipe sees data immediately.
    std::cout.setf(std::ios::unitbuf);

    try {
        const std::string input = readStdin();

        if (input.empty()) {
            respondError("Empty request: expected a JSON object on stdin");
            return 1;
        }

        std::unordered_map<std::string, std::string> req;
        try {
            req = mse::json::parseObject(input);
        } catch (const std::exception& e) {
            respondError(std::string("JSON parse error: ") + e.what());
            return 1;
        }

        // --- Validate required fields ---
        if (req.find("mode") == req.end() || req.at("mode").empty()) {
            respondError("Missing required field: mode");
            return 1;
        }
        if (req.find("sequenceA") == req.end()) {
            respondError("Missing required field: sequenceA");
            return 1;
        }
        if (req.find("sequenceB") == req.end()) {
            respondError("Missing required field: sequenceB");
            return 1;
        }

        const std::string mode      = req.at("mode");
        const std::string sequenceA = req.at("sequenceA");
        const std::string sequenceB = req.at("sequenceB");

        // --- Hard length cap: defence-in-depth (Node layer also validates) ---
        constexpr std::size_t kMaxLen = 10000;
        if (sequenceA.size() > kMaxLen || sequenceB.size() > kMaxLen) {
            respondError("Sequence length exceeds maximum allowed (10000 characters)");
            return 1;
        }

        // --- Dispatch to requested algorithm ---
        if (mode == "lcs") {
            respond(mse::computeLCS(sequenceA, sequenceB));

        } else if (mode == "edit-distance") {
            respond(mse::computeEditDistance(sequenceA, sequenceB));

        } else if (mode == "needleman-wunsch") {
            const mse::ScoringParams params = buildScoringParams(req);
            respond(mse::computeNeedlemanWunsch(sequenceA, sequenceB, params));

        } else if (mode == "smith-waterman") {
            const mse::ScoringParams params = buildScoringParams(req);
            respond(mse::computeSmithWaterman(sequenceA, sequenceB, params));

        } else {
            respondError("Unknown mode: \"" + mode +
                         "\". Valid values: lcs, edit-distance, needleman-wunsch, smith-waterman");
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        respondError(std::string("Unhandled engine exception: ") + e.what());
        return 1;
    } catch (...) {
        respondError("Unknown fatal error in engine");
        return 1;
    }
}
