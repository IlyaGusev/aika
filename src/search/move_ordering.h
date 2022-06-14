#pragma once

#include <strategy.h>
#include <evaluation.h>
#include <search/history_heuristics.h>
#include <search/transposition_table.h>

#include <algorithm>


class TMoveOrdering {
private:
    const lczero::Position& Position;
    const lczero::Move& PrevMove;
    std::optional<lczero::Move> TTMove;

    const THistoryHeuristics* HistoryHeuristics;

public:
    TMoveOrdering(
        const lczero::Position& position,
        const lczero::Move& prevMove,
        std::optional<lczero::Move> ttMove,
        const THistoryHeuristics* historyHeuristics
    )
        : Position(position)
        , PrevMove(prevMove)
        , TTMove(ttMove)
        , HistoryHeuristics(historyHeuristics)
    {}

    std::vector<TMoveInfo> Order(const lczero::MoveList& moves) {
        std::vector<TMoveInfo> movesScores;
        movesScores.reserve(moves.size());
        for (const lczero::Move& move : moves) {
            movesScores.emplace_back(move, CalcMoveOrder(move));
        }
        std::stable_sort(movesScores.begin(), movesScores.end());
        return movesScores;
    }

private:
    int CalcMoveOrder(const lczero::Move& move) const;
};
