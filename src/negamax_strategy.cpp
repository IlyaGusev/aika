#include "negamax_strategy.h"
#include "evaluation.h"

#include <algorithm>
#include <chrono>
#include <iostream>

TMoveInfo TNegamaxStrategy::NegaMax(
    TNode& node
) const {
    const auto& position = node.Position;
    const auto& move = node.Move;
    const auto& depth = node.Depth;

    if (depth <= 0 || IsTerminal(position, true)) {
        return TMoveInfo{move, Evaluate(position)};
    }

    const auto& board = position.GetBoard();
    const auto& beta = node.Beta;
    int alpha = node.Alpha;

    std::vector<TNode> children;
    std::vector<lczero::Move> legalMoves = board.GenerateLegalMoves();
    children.reserve(legalMoves.size());
    for (const auto& newMove : legalMoves) {
        children.emplace_back(position, newMove, depth - 1, -beta, -alpha);
    }

    std::sort(children.begin(), children.end(), [](const TNode& f, const TNode& s) {
        return Evaluate(f.Position, true) > Evaluate(s.Position, true);
    });

    TMoveInfo bestMove(lczero::Move(), MIN_SCORE_VALUE);
    for (auto& child : children) {
        child.Beta = -alpha;
        TMoveInfo move = NegaMax(child);
        move.Move = child.Move;
        move.Score = -move.Score;
        node.TreeNodesCount += child.TreeNodesCount;
        if (move > bestMove) {
            bestMove = move;
        }
        alpha = std::max(alpha, move.Score);
        if (alpha > beta) {
            break;
        }
    }

    return bestMove;
}

std::optional<TMoveInfo> TNegamaxStrategy::MakeMove(
    const lczero::PositionHistory& history
) const {
    const size_t DEPTH = 5;
    auto timerStart = std::chrono::high_resolution_clock::now();
    TNode startNode{
        history.Last(),
        DEPTH,
        MIN_SCORE_VALUE,
        MAX_SCORE_VALUE
    };
    TMoveInfo move = NegaMax(startNode);
    auto timerEnd = std::chrono::high_resolution_clock::now();

    move.NodesCount = startNode.TreeNodesCount;
    move.TimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timerEnd - timerStart).count();
    move.Depth = DEPTH;
    return move;
}
