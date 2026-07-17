#pragma once

#include <json/value.h>
#include <json/reader.h>

#include <string>

struct TSearchConfig {
    int Depth = 3;
    int QuiescenceSearchDepth = 0;
    bool EnableAlphaBeta = false;
    bool EnablePST = false;
    bool EnableTT = false;
    bool EnableNullMove = false;
    int NullMoveDepthReduction = 3;
    int NullMoveEvalMargin = 0;
    bool EnableLMR = false;
    int LMRMinMoveNumber = 7;
    int LMRMinPly = 2;
    int LMRMinDepth = 2;
    int LMRReduction = 1;
    bool EnableKillers = true;
    bool EnableHH = false;
    // Root budgets below this keep conservative pruning (exact tactics);
    // deeper budgets enable the aggressive stack
    int AggressiveDepthMin = 12;
    // Soft cap for iterative deepening: no new iteration starts after this
    // many ms (0 = off); the running iteration always completes
    int TimeLimitMs = 0;
    bool EnableSEESkip = false;
    int SEESkipQuietMargin = 0;

    TSearchConfig() = default;
    explicit TSearchConfig(const std::string& configFile) {
        Parse(configFile);
    }

    void Parse(const std::string& configFile);
};


