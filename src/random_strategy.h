#pragma once

#include "strategy.h"

class TRandomStrategy : public IStrategy {
public:
    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) override;

    virtual const char* GetName() const override {
        return "Random";
    }
};
