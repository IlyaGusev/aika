#pragma once

#include "strategy.h"

#include <iostream>

const int MIN_SCORE_VALUE = -100000;
const int MAX_SCORE_VALUE = 100000;

struct TNode {
    lczero::Position Position;
    lczero::Move Move;
    size_t Depth = 0;
    int Alpha = MIN_SCORE_VALUE;
    int Beta = MAX_SCORE_VALUE;
    size_t TreeNodesCount = 1;

    TNode(
        const lczero::Position& oldPosition,
        const lczero::Move& move,
        size_t depth,
        int alpha,
        int beta
    )
        : Position(oldPosition, move)
        , Move(move)
        , Depth(depth)
        , Alpha(alpha)
        , Beta(beta)
    {}

    TNode(
        const lczero::Position& position,
        size_t depth,
        int alpha,
        int beta
    )
        : Position(position)
        , Move()
        , Depth(depth)
        , Alpha(alpha)
        , Beta(beta)
    {}
};

class TNegamaxStrategy : public IStrategy {
public:
    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) const override;

    virtual const char* GetName() const override {
        return "Negamax";
    }

private:
    TMoveInfo NegaMax(TNode& node) const;
};
