#pragma once

#include <strategy.h>
#include <util.h>

#include <unordered_map>

class TTranspositionTable {
public:
    enum class ENodeType {
        Unknown,
        PV,
        Cut
    };

    struct TNode {
        size_t Depth;
        TMoveInfo BestMove;
        ENodeType Type;

        TNode(size_t depth, TMoveInfo bestMove, ENodeType type)
            : Depth(depth), BestMove(bestMove), Type(type)
        {}
    };

    struct TPositionHasher {
        uint64_t operator() (const lczero::Position& pos) const {
            return pos.Hash();
        }
    };

    struct TPositionEqualFn {
        bool operator() (
            const lczero::Position& p1,
            const lczero::Position& p2
        ) const {
            return p1.GetBoard() == p2.GetBoard() && p1.GetRepetitions() == p2.GetRepetitions();
        }
    };

private:
    std::unordered_map<lczero::Position, TNode, TPositionHasher, TPositionEqualFn> Data;

public:
    TTranspositionTable() {}

    void Insert(
        const lczero::Position& position,
        const TMoveInfo& bestMove,
        size_t depth,
        ENodeType type
    );
    std::optional<TMoveInfo> Find(
        const lczero::Position& position,
        size_t depth = 0,
        ENodeType type = ENodeType::PV
    ) const;
};
