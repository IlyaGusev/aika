#include "negamax_strategy.h"
#include "evaluation.h"

#include <algorithm>
#include <chrono>
#include <iostream>


TMoveInfo TNegamaxStrategy::Search(
    TNode& node
) const {
    const lczero::Position& position = node.Position;
    const size_t depth = node.Depth;
    const bool isCapturesOnly = node.IsCapturesOnly;

    auto it = TranspositionTable.find(position);
    if (it != TranspositionTable.end() && it->second.Depth >= depth) {
        return it->second.BestMove;
    }

    const int staticScore = Evaluate(position);
    if (IsTerminal(position, true) || (depth == 0 && isCapturesOnly)) {
        TMoveInfo bestMove{lczero::Move(), staticScore};
        InsertTransposition(node, bestMove);
        return bestMove;
    }
    if (depth == 0 && !isCapturesOnly) {
        TNode quiscenceNode(position, 1, node.Alpha, node.Beta, true);
        TMoveInfo bestMove = Search(quiscenceNode);
        node.TreeNodesCount += quiscenceNode.TreeNodesCount;
        InsertTransposition(node, bestMove);
        return bestMove;
    }

    const auto& board = position.GetBoard();
    const int beta = node.Beta;
    int alpha = node.Alpha;

    std::vector<TNode> children;
    std::vector<lczero::Move> legalMoves = board.GenerateLegalMoves();
    children.reserve(legalMoves.size());
    for (const auto& newMove : legalMoves) {
        if (isCapturesOnly && !IsCapture(position, newMove)) {
            continue;
        }
        children.emplace_back(position, newMove, depth - 1, -beta, -alpha, isCapturesOnly);
    }

    if (children.empty()) {
        TMoveInfo bestMove(lczero::Move(), staticScore);
        return bestMove;
    }

    std::stable_sort(children.begin(), children.end(), [&](const TNode& f, const TNode& s) {
        auto it1 = TranspositionTable.find(f.Position);
        auto it2 = TranspositionTable.find(s.Position);
        int score1 = (it1 != TranspositionTable.end()) ? it1->second.BestMove.Score : MAX_SCORE_VALUE;
        int score2 = (it2 != TranspositionTable.end()) ? it2->second.BestMove.Score : MAX_SCORE_VALUE;
        return score1 < score2;
    });

    TMoveInfo bestMove(lczero::Move(), MIN_SCORE_VALUE);
    for (auto& child : children) {
        child.Beta = -alpha;
        TMoveInfo enemyMove = Search(child);
        TMoveInfo ourMove(child.Move, -enemyMove.Score);
        node.TreeNodesCount += child.TreeNodesCount;
        bestMove = std::max(bestMove, ourMove);
        alpha = std::max(alpha, ourMove.Score);
        if (alpha > beta) {
            break;
        }
    }
    InsertTransposition(node, bestMove);
    return bestMove;
}

void TNegamaxStrategy::InsertTransposition(
    const TNode& node,
    const TMoveInfo& bestMove
) const {
    if (node.IsCapturesOnly) {
        return;
    }
    auto it = TranspositionTable.find(node.Position);
    if (it == TranspositionTable.end() || it->second.Depth < node.Depth) {
        TranspositionTable.insert_or_assign(node.Position, TTTNode(node.Depth, bestMove));
    }
}

std::optional<TMoveInfo> TNegamaxStrategy::MakeMove(
    const lczero::PositionHistory& history
) const {
    TranspositionTable.reserve(10000000);
    const size_t DEPTH = 6;
    auto timerStart = std::chrono::high_resolution_clock::now();
    size_t currentDepth = 0;
    TMoveInfo move;
    while (currentDepth <= DEPTH) {
        TNode startNode{
            history.Last(),
            currentDepth,
            MIN_SCORE_VALUE,
            MAX_SCORE_VALUE
        };
        move = Search(startNode);
        auto timerEnd = std::chrono::high_resolution_clock::now();
        move.NodesCount = startNode.TreeNodesCount;
        move.TimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timerEnd - timerStart).count();
        move.Depth = currentDepth;
        currentDepth += 1;
    }
    return move;
}
