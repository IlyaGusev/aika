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
    bool NullMoveDepthReduction = 3;
    bool EnableLMR = false;
    bool EnableHH = false;

    TSearchConfig() = default;
    explicit TSearchConfig(const std::string& configFile) {
        Parse(configFile);
    }

    void Parse(const std::string& configFile);
};


