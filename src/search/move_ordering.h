#pragma once

#include <strategy.h>
#include <evaluation.h>
#include <search/history_heuristics.h>
#include <search/transposition_table.h>
#include <search/killer_moves.h>

#include <algorithm>


class TMoveOrdering {
private:
    const lczero::Position& Position;
    const lczero::Move& PrevMove;
    std::optional<lczero::Move> TTMove;
    const TKillerMoves* KillerMoves;
    const THistoryHeuristics* HistoryHeuristics;

public:
    TMoveOrdering(
        const lczero::Position& position,
        const lczero::Move& prevMove,
        std::optional<lczero::Move> ttMove,
        const TKillerMoves* killerMoves,
        const THistoryHeuristics* historyHeuristics
    )
        : Position(position)
        , PrevMove(prevMove)
        , TTMove(ttMove)
        , KillerMoves(killerMoves)
        , HistoryHeuristics(historyHeuristics)
    {}

    std::vector<TMoveInfo> Order(const lczero::MoveList& moves, size_t ply) const {
        std::vector<TMoveInfo> movesScores;
        movesScores.reserve(moves.size());
        for (const lczero::Move& move : moves) {
            movesScores.emplace_back(move, CalcMoveOrder(move, ply));
        }
        std::stable_sort(movesScores.begin(), movesScores.end(),
            [](const TMoveInfo & a, const TMoveInfo& b) -> bool {
                if (a.Score < b.Score) return true;
                if (a.Score > b.Score) return false;
                return (a.Move.as_packed_int() < b.Move.as_packed_int());
            }
        );
        /*for (const auto& move : movesScores) {
            std::cerr << move.Score << " ";
        }
        std::cerr << std::endl;*/
        return movesScores;
    }

private:
    int CalcMoveOrder(const lczero::Move& move, size_t ply) const;
};
