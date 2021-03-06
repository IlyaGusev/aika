#pragma once

#include "strategy.h"

#include <iostream>
#include <unordered_map>

const int MIN_SCORE_VALUE = -100000;
const int MAX_SCORE_VALUE = 100000;

struct TNode {
    lczero::Position Position;
    lczero::Move Move;
    size_t Depth = 0;
    int Alpha = MIN_SCORE_VALUE;
    int Beta = MAX_SCORE_VALUE;
    size_t TreeNodesCount = 1;
    bool IsCapturesOnly = false;

    TNode(
        const lczero::Position& oldPosition,
        const lczero::Move& move,
        size_t depth,
        int alpha,
        int beta,
        bool isCapturesOnly
    )
        : Position(oldPosition, move)
        , Move(move)
        , Depth(depth)
        , Alpha(alpha)
        , Beta(beta)
        , IsCapturesOnly(isCapturesOnly)
    {}

    TNode(
        const lczero::Position& position,
        size_t depth,
        int alpha = MIN_SCORE_VALUE,
        int beta = MAX_SCORE_VALUE,
        bool isCapturesOnly = false
    )
        : Position(position)
        , Move()
        , Depth(depth)
        , Alpha(alpha)
        , Beta(beta)
        , IsCapturesOnly(isCapturesOnly)
    {}
};

struct TTTNode {
    size_t Depth;
    TMoveInfo BestMove;

    TTTNode(size_t depth, TMoveInfo bestMove)
    : Depth(depth)
    , BestMove(bestMove)
    {}
};

struct TPositionHasher {
    uint64_t operator() (const lczero::Position& pos) const {
        return pos.Hash();
    }
};

struct TPositionEqualFn {
    bool operator() (const lczero::Position& p1, const lczero::Position& p2) const {
        return p1.GetBoard() == p2.GetBoard() && p1.GetRepetitions() == p2.GetRepetitions();
    }
};

using TTranspositionTable = std::unordered_map<lczero::Position, TTTNode, TPositionHasher, TPositionEqualFn>;

class TNegamaxStrategy : public IStrategy {
private:
    mutable TTranspositionTable TranspositionTable;

public:
    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) const override;

    virtual const char* GetName() const override {
        return "Negamax";
    }

private:
    TMoveInfo Search(
        TNode& node
    ) const;

    void InsertTransposition(
        const TNode& node,
        const TMoveInfo& bestMove
    ) const;

};
