#pragma once

#include <strategy.h>

class TRandomStrategy : public IStrategy {
public:
    std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) override;

    const char* GetName() const override {
        return "Random";
    }
};
