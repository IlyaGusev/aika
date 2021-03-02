#pragma once

#include "strategy.h"

class TGreedyStrategy : public IStrategy {
public:
    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) const override;

    virtual const char* GetName() const override {
        return "Greedy";
    }
};
