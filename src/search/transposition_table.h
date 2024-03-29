#pragma once

#include <strategy.h>
#include <util.h>

#include <boost/unordered_map.hpp>

class TTranspositionTable {
public:
    enum class ENodeType : short {
        Unknown = -1,
        PV = 0,
        Cut,
        All,
        Count
    };

    struct TNode {
        int Depth = 0;
        TMoveInfo Move;
        ENodeType Type = ENodeType::Unknown;

        explicit TNode(int depth, const TMoveInfo& move, ENodeType type)
            : Depth(depth)
            , Move(move)
            , Type(type)
        {}
    };

    struct TPositionHasher {
        uint64_t operator() (const lczero::Position& pos) const {
            return pos.GetBoard().Hash();
        }
    };

    struct TPositionEqualFn {
        bool operator() (
            const lczero::Position& p1,
            const lczero::Position& p2
        ) const {
            return p1.GetBoard() == p2.GetBoard();
        }
    };

private:
    boost::unordered_map<lczero::Position, TNode, TPositionHasher, TPositionEqualFn> Data;
    boost::unordered_map<ENodeType, size_t> InsertCounts;
    mutable boost::unordered_map<ENodeType, size_t> FindCounts;

public:
    TTranspositionTable() {}

    void Insert(
        const lczero::Position& position,
        const TMoveInfo& move,
        int depth,
        int alpha,
        int beta
    );

    std::optional<TNode> Find(
        const lczero::Position& position,
        int depth = 0
    ) const;

    std::string GetStats() const;

    void Clear();

private:
    void Insert(
        const lczero::Position& position,
        const TMoveInfo& move,
        int depth,
        ENodeType type
    );

    const char* ToString(ENodeType type) const;
};
