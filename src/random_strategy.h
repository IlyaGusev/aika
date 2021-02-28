#pragma once

#include "strategy.h"

class TRandomStrategy : public IStrategy {
public:
    virtual std::optional<lczero::Move> MakeMove(
        const lczero::PositionHistory& history
    ) const override;

    virtual const char* GetName() const override {
        return "Random";
    }
};
