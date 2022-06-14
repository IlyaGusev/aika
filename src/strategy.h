#pragma once

#include <chess/position.h>

#include <vector>
#include <memory>
#include <optional>

struct TMoveInfo {
    lczero::Move Move;
    int Score = 0.0;
    size_t NodesCount = 0;
    size_t TimeMs = 0;
    int Depth = 0; // Negative values in QSearch

    TMoveInfo() = default;
    explicit TMoveInfo(const lczero::Move& move) : Move(move) {}
    TMoveInfo(int score) : Move(), Score(score) {}
    TMoveInfo(const lczero::Move& move, int score) : Move(move), Score(score) {}
    TMoveInfo(const TMoveInfo& other) = default;
    TMoveInfo(TMoveInfo&& other) = default;
    TMoveInfo& operator=(const TMoveInfo& other) = default;

    bool operator<(const TMoveInfo& other) const {
        return Score < other.Score;
    }

    bool operator>(const TMoveInfo& other) const {
        return Score > other.Score;
    }

    bool operator==(const TMoveInfo& other) const {
        return Score == other.Score && Move == other.Move;
    }
};

class IStrategy {
public:
    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) = 0;

    virtual const char* GetName() const = 0;
};

using TStrategies = std::vector<std::unique_ptr<IStrategy>>;
