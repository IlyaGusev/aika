#include "greedy_strategy.h"
#include "evaluation.h"

#include <limits>
#include <iostream>

std::optional<TMoveInfo> TGreedyStrategy::MakeMove(
    const lczero::PositionHistory& history
) const {
    const auto& legalMoves = history.Last().GetBoard().GenerateLegalMoves();
    if (legalMoves.empty()) {
        return std::nullopt;
    }

    lczero::Move bestMove;
    double bestScore = std::numeric_limits<double>::lowest();
    for (const auto& move : legalMoves) {
        lczero::Position position(history.Last(), move);
        double score = Evaluate(position, true);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    return TMoveInfo(bestMove, bestScore);
}
