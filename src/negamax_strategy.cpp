#include "negamax_strategy.h"
#include "evaluation.h"

#include <iostream>

TMoveInfo TNegamaxStrategy::NegaMax(
    const lczero::Position& position,
    size_t depth,
    double alpha,
    double beta
) const {
    if (depth <= 0 || IsTerminal(position, true)) {
        return TMoveInfo{lczero::Move(), Evaluate(position)};
    }
    lczero::Move bestMove;
    double bestScore = std::numeric_limits<double>::lowest();
    const auto legalMoves = position.GetBoard().GenerateLegalMoves();
    for (const auto& move : legalMoves) {
        lczero::Position childPosition(position, move);
        TMoveInfo mInfo = NegaMax(childPosition, depth - 1, -beta, -alpha);
        double score = -mInfo.Score;
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break;
        }
    }
    return TMoveInfo{bestMove, bestScore};
}

std::optional<TMoveInfo> TNegamaxStrategy::MakeMove(
    const lczero::PositionHistory& history
) const {
    return NegaMax(
        history.Last(),
        4,
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::max()
    );
}
