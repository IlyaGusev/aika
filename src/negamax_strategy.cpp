#include "negamax_strategy.h"
#include "evaluation.h"
#include "search/see.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>

TMoveInfo TNegamaxStrategy::Search(
    TNode& node,
    int alpha,
    int beta
) {
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const size_t depth = node.Depth;

    std::optional<TMoveInfo> moveInfo = TranspositionTable.Find(position, depth);
    if (moveInfo) {
        return *moveInfo;
    }

    const int staticScore = Evaluate(position, Config.EnablePST);
    if (IsTerminal(position)) {
        TMoveInfo moveInfo(staticScore);
        return {staticScore};
    }

    if (depth == 0) {
        if (Config.QuiescenceSearchDepth != 0) {
            TNode qNode(position, Config.QuiescenceSearchDepth);
            return QuiescenceSearch(qNode, alpha, beta);
        }
        return {staticScore};
    }

    std::multimap<int, TNode> children;
    for (const auto& move : board.GenerateLegalMoves()) {
        int score = CalcMoveOrder(position, move);
        children.emplace(score, TNode(position, move, depth - 1));
    }

    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (auto& [_, child] : children) {
        const auto& bestEnemyMove = Search(child, -beta, -alpha);
        const int bestEnemyScore = bestEnemyMove.Score;
        node.TreeNodesCount += child.TreeNodesCount;

        const auto& ourMove = child.Move;
        const int ourScore = -bestEnemyScore;
        TMoveInfo ourMoveInfo(ourMove, ourScore);
        //if (depth == 5) {
        //    std::cerr << ourMove.as_string() << " " << ourScore << " " << alpha << " " << beta << " " << bestMoveInfo.Move.as_string() << " " <<  bestMoveInfo.Score << std::endl;
        //}
        if (Config.EnableAlphaBeta && ourScore >= beta) {
            if (Config.EnableTT) {
                TranspositionTable.Insert(position, ourMoveInfo, depth, TTranspositionTable::ENodeType::Cut);
            }
            if (!IsCapture(position, ourMove)) {
                int side = position.IsBlackToMove();
                EPieceType fromPiece = GetPieceType(board, ourMove.from());
                HistoryHeuristics.Add(side, fromPiece, ourMove.to(), depth);
            }
            return ourMoveInfo;
        }
        if (ourMoveInfo > bestMoveInfo) {
            bestMoveInfo = ourMoveInfo;
            alpha = std::max(alpha, ourScore);
        }
    }
    if (Config.EnableTT) {
        TranspositionTable.Insert(position, bestMoveInfo, depth, TTranspositionTable::ENodeType::PV);
    }
    return bestMoveInfo;
}

TMoveInfo TNegamaxStrategy::QuiescenceSearch(
    TNode& node,
    int alpha,
    int beta
) {
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const size_t depth = node.Depth;

    const int staticScore = Evaluate(position, Config.EnablePST);
    if (IsTerminal(position) || (depth == 0)) {
        return {staticScore};
    }
    if (Config.EnableAlphaBeta && staticScore >= beta) {
        //return {staticScore};
        return {beta};
    }
    alpha = std::max(alpha, staticScore);

    std::multimap<int, TNode> children;
    for (const auto& move : board.GenerateLegalMoves()) {
        if (IsCapture(position, move) && EvaluateCaptureSEE(position, move) >= 0) {
            int score = CalcMoveOrder(position, move);
            children.emplace(score, TNode(position, move, depth - 1));
        }
    }

    if (children.empty()) {
        return {staticScore};
    }

    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (auto& [_, child] : children) {
        const auto& bestEnemyMove = QuiescenceSearch(child, -beta, -alpha);
        const int bestEnemyScore = bestEnemyMove.Score;
        node.TreeNodesCount += child.TreeNodesCount;

        const auto& ourMove = child.Move;
        const int ourScore = -bestEnemyScore;
        TMoveInfo ourMoveInfo(ourMove, ourScore);

        if (Config.EnableAlphaBeta && ourScore >= beta) {
            //return ourMoveInfo;
            return {ourMove, beta};
        }
        if (ourMoveInfo > bestMoveInfo) {
            bestMoveInfo = ourMoveInfo;
            alpha = std::max(alpha, ourScore);
        }
    }
    return bestMoveInfo;
}

int TNegamaxStrategy::CalcMoveOrder(
    const lczero::Position& position,
    const lczero::Move& move
) const {
    const int stageDiff = 10000;

    // Hash move
    int stage = -1;
    std::optional<TMoveInfo> bm = TranspositionTable.Find(position);
    if (bm && bm->Move == move) {
        return stage * stageDiff;
    }

    // Promotions
    stage = 0;
    if (move.promotion() != lczero::Move::Promotion::None) {
        return stage * stageDiff;
    }

    // Captures
    if (IsCapture(position, move)) {
        int captureValue = EvaluateCaptureSEE(position, move);
        if (captureValue > 0) {
            stage = 1;
        } else if (captureValue == 0) {
            stage = 2;
        } else {
            stage = 4;
        }
        return stage * stageDiff - captureValue;
    }

    // History heuristics
    stage = 3;
    EPieceType fromPiece = GetPieceType(position.GetBoard(), move.from());
    int side = position.IsBlackToMove();
    int historyScore = HistoryHeuristics.Get(side, fromPiece, move.to());
    if (historyScore != 0) {
        return stage * stageDiff - historyScore;
    }

    stage = 5;
    return stage * stageDiff;
}

std::optional<TMoveInfo> TNegamaxStrategy::MakeMove(
    const lczero::PositionHistory& history
) {
    auto timerStart = std::chrono::high_resolution_clock::now();
    size_t currentDepth = 0;
    HistoryHeuristics.Clear();
    TMoveInfo move;
    while (currentDepth <= Config.Depth) {
        TNode startNode{
            history.Last(),
            currentDepth,
        };
        move = Search(startNode, MIN_SCORE_VALUE, MAX_SCORE_VALUE);
        auto timerEnd = std::chrono::high_resolution_clock::now();
        move.NodesCount = startNode.TreeNodesCount;
        move.TimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timerEnd - timerStart).count();
        move.Depth = currentDepth;
        if (move.Score == MIN_SCORE_VALUE || move.Score == MAX_SCORE_VALUE) {
            break;
        }
        currentDepth += 1;
    }
    return move;
}
