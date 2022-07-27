#pragma once

#include <strategy.h>
#include <util.h>
#include <search/config.h>
#include <search/history_heuristics.h>
#include <search/transposition_table.h>
#include <search/killer_moves.h>

#include <unordered_map>
#include <string>

struct TSearchNode {
    lczero::Position Position;
    lczero::Move Move;
    int Depth = 0;
    size_t Ply = 0;
    mutable size_t TreeNodesCount = 1;

    TSearchNode(
        const lczero::Position& oldPosition,
        const lczero::Move& move,
        int depth,
        size_t ply
    )
        : Position(oldPosition, move)
        , Move(move)
        , Depth(depth)
        , Ply(ply)
    {}

    TSearchNode(const lczero::Position& position, int depth, size_t ply)
        : Position(position)
        , Move()
        , Depth(depth)
        , Ply(ply)
    {}
};

struct TBetaCutoffStats {
    std::unordered_map<size_t, size_t> PositionsCounts;

    void Increment(size_t position) { PositionsCounts[position] += 1; }
    std::string GetStats() const;
};


class TSearchStrategy : public IStrategy {
public:
    explicit TSearchStrategy(
        const std::string& configFile
    ) : Config(configFile) {}

    explicit TSearchStrategy(
        const TSearchConfig& config
    ) : Config(config) {}

    std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) override;

    const char* GetName() const override {
        return "Search";
    }

private:
    TMoveInfo Search(
        const TSearchNode& node,
        int alpha,
        int beta
    );

    std::optional<TMoveInfo> TryNullMove(
        const TSearchNode& node,
        int beta,
        int staticScore
    );

    std::optional<TMoveInfo> TryLMR(
        const TSearchNode& node,
        const lczero::Move& move,
        int alpha,
        int beta,
        size_t moveNumber
    );

    TMoveInfo QuiescenceSearch(
        const TSearchNode& node,
        int alpha,
        int beta
    );

private:
    TTranspositionTable TranspositionTable;
    THistoryHeuristics HistoryHeuristics;
    TSearchConfig Config;
    TKillerMoves KillerMoves;
    TBetaCutoffStats BetaCutoffStats;
};
