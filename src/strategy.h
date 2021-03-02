#pragma once

#include <chess/position.h>

#include <vector>
#include <memory>
#include <optional>

struct TMoveInfo {
    lczero::Move Move;
    double Score = 0.0;

    TMoveInfo(const lczero::Move& move) : Move(move) {}
    TMoveInfo(const lczero::Move& move, double score) : Move(move), Score(score) {}
    TMoveInfo(const TMoveInfo& other) : Move(other.Move), Score(other.Score) {}
};

class IStrategy {
public:
    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) const = 0;

    virtual const char* GetName() const = 0;
};

using TStrategies = std::vector<std::unique_ptr<IStrategy>>;
