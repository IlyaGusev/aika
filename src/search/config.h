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
    size_t NullMoveDepthReduction = 3;
    size_t NullMoveEvalMargin = 0;
    bool EnableLMR = false;
    size_t LMRMinMoveNumber = 15;
    size_t LMRMinPly = 2;
    size_t LMRMinDepth = 2;
    bool EnableKillers = false;
    bool EnableHH = false;

    TSearchConfig() = default;
    explicit TSearchConfig(const std::string& configFile) {
        Parse(configFile);
    }

    void Parse(const std::string& configFile);
};


