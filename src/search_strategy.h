#pragma once

#include <strategy.h>
#include <util.h>
#include <pst.h>
#include <search/config.h>
#include <search/history_heuristics.h>
#include <search/transposition_table.h>
#include <search/killer_moves.h>

#include <chrono>
#include <string>

struct TSearchNode {
    lczero::Position Position;
    lczero::Move Move;
    uint64_t HashValue;
    int Depth = 0;
    size_t Ply = 0;
    mutable size_t TreeNodesCount = 1;
    // -1 unknown, else cached result of IsUnderCheck for this node's side
    mutable int8_t InCheckFlag = -1;

    TPstState Pst;

    bool IsUnderCheck() const {
        if (InCheckFlag < 0) {
            InCheckFlag = Position.GetBoard().IsUnderCheck() ? 1 : 0;
        }
        return InCheckFlag != 0;
    }

    TSearchNode(
        const TSearchNode& parent,
        const lczero::Move& move,
        int depth,
        size_t ply
    )
        : Position(parent.Position, move)
        , Move(move)
        , HashValue(Position.GetBoard().Hash())
        , Depth(depth)
        , Ply(ply)
        , Pst(ApplyMovePst(parent.Pst, parent.Position.GetBoard(), move))
    {}

    TSearchNode(const lczero::Position& position, int depth, size_t ply)
        : Position(position)
        , Move()
        , HashValue(Position.GetBoard().Hash())
        , Depth(depth)
        , Ply(ply)
        , Pst(CalcPstState(Position))
    {}

    // Null-move child: same physical position, flipped perspective
    TSearchNode(const lczero::Position& position, const TPstState& parentPst,
                int depth, size_t ply)
        : Position(position)
        , Move()
        , HashValue(Position.GetBoard().Hash())
        , Depth(depth)
        , Ply(ply)
        , Pst{{parentPst.Packed[1], parentPst.Packed[0]}, parentPst.Phase}
    {}
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

    void SetTimeLimit(int timeLimitMs) {
        Config.TimeLimitMs = timeLimitMs;
    }

    void SetDepth(int depth) {
        Config.Depth = depth;
    }

    const char* GetName() const override {
        return "Search";
    }

private:
    TMoveInfo Search(
        const TSearchNode& node,
        int alpha,
        int beta
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

    // Hard deadline (TimeLimitMs after search start, active only when > 0):
    // an iteration running past it is aborted and its result discarded
    std::chrono::steady_clock::time_point Deadline;
    bool DeadlineEnabled = false;
    bool Aborted = false;
    size_t NodesSinceTimeCheck = 0;
};
