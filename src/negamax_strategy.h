#pragma once

#include "strategy.h"

class TNegamaxStrategy : public IStrategy {
public:
    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) const override;

    virtual const char* GetName() const override {
        return "Negamax";
    }

private:
    TMoveInfo NegaMax(
        const lczero::Position& position,
        size_t depth,
        double alpha,
        double beta
    ) const;
};
