#include <search_strategy.h>

#include <evaluation.h>
#include <search/see.h>

#include <algorithm>
#include <chrono>
#include <map>

TMoveInfo TSearchStrategy::Search(
    const TSearchNode& node,
    int alpha,
    int beta
) {
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const size_t depth = node.Depth;
    const size_t ply = node.Ply;

    const auto moveInfo = TranspositionTable.Find(position, depth);
    if (moveInfo) {
        return *moveInfo;
    }

    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

    const int staticScore = Evaluate(
        position, ourLegalMoves,
        theirLegalMoves, Config.EnablePST);

    if (IsTerminal(position, ourLegalMoves, theirLegalMoves)) {
        return {staticScore};
    }

    if (depth == 0) {
        if (Config.QuiescenceSearchDepth != 0) {
            TSearchNode qNode(position, Config.QuiescenceSearchDepth, 0);
            return QuiescenceSearch(qNode, alpha, beta);
        }
        return {staticScore};
    }

    std::multimap<int, TSearchNode> children;
    for (const auto& move : ourLegalMoves) {
        int score = CalcMoveOrder(position, move, ply);
        children.emplace(score, TSearchNode(position, move, depth - 1, ply + 1));
    }

    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (auto& [_, child] : children) {
        const auto& bestEnemyMove = Search(child, -beta, -alpha);
        const int bestEnemyScore = bestEnemyMove.Score;
        node.TreeNodesCount += child.TreeNodesCount;

        const auto& ourMove = child.Move;
        const int ourScore = -bestEnemyScore;
        TMoveInfo ourMoveInfo(ourMove, ourScore);
        if (Config.EnableAlphaBeta && ourScore >= beta) {
            if (Config.EnableTT) {
                TranspositionTable.Insert(position, ourMoveInfo, depth,
                    TTranspositionTable::ENodeType::Cut);
            }
            if (!IsCapture(position, ourMove)) {
                KillerMoves[ply] = ourMove;
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
        TranspositionTable.Insert(position, bestMoveInfo, depth,
            TTranspositionTable::ENodeType::PV);
    }
    return bestMoveInfo;
}

TMoveInfo TSearchStrategy::QuiescenceSearch(
    const TSearchNode& node,
    int alpha,
    int beta
) {
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();
    const size_t depth = node.Depth;
    const size_t ply = node.Ply;

    const int staticScore = Evaluate(
        position, ourLegalMoves,
        theirLegalMoves, Config.EnablePST);

    if (IsTerminal(position, ourLegalMoves, theirLegalMoves) || (depth == 0)) {
        return {staticScore};
    }
    if (Config.EnableAlphaBeta && staticScore >= beta) {
        return {beta};
    }
    alpha = std::max(alpha, staticScore);

    std::multimap<int, TSearchNode> children;
    for (const auto& move : ourLegalMoves) {
        if (!IsCapture(position, move)) {
            continue;
        }
        if (EvaluateCaptureSEE(position, move) >= 0) {
            int score = CalcMoveOrder(position, move, ply);
            children.emplace(score, TSearchNode(position, move, depth - 1, ply + 1));
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
            return {ourMove, beta};
        }
        if (ourMoveInfo > bestMoveInfo) {
            bestMoveInfo = ourMoveInfo;
            alpha = std::max(alpha, ourScore);
        }
    }
    return bestMoveInfo;
}

int TSearchStrategy::CalcMoveOrder(
    const lczero::Position& position,
    const lczero::Move& move,
    size_t ply
) const {
    UNUSED(ply);
    const int stageDiff = 10000;

    // Hash move
    int stage = -1;
    std::optional<TMoveInfo> bm = TranspositionTable.Find(position);
    if (bm && bm->Move == move) {
        return stage * stageDiff;
    }

    // Promotions
    stage = 0;
    const auto promotion = move.promotion();
    if (promotion != lczero::Move::Promotion::None) {
        if (promotion == lczero::Move::Promotion::Queen) {
            return stage * stageDiff - GetPieceValue(EPieceType::Queen);
        }
        return stage * stageDiff;
    }

    //stage = 1;
    //lczero::Position newPosition(position, move);
    //if (newPosition.GetBoard().IsUnderCheck()) {
    //    return stage * stageDiff;
    //}

    // Captures
    if (IsCapture(position, move)) {
        int captureValue = EvaluateCaptureSEE(position, move);
        if (captureValue > 0) {
            stage = 2;
        } else if (captureValue == 0) {
            stage = 3;
        } else {
            stage = 6;
        }
        return stage * stageDiff - captureValue;
    }

    // Killer move
    //stage = 3;
    //if (KillerMoves[ply] == move) {
    //    return stage * stageDiff;
    //}

    // History heuristics
    stage = 5;
    EPieceType fromPiece = GetPieceType(position.GetBoard(), move.from());
    int side = position.IsBlackToMove();
    int historyScore = HistoryHeuristics.Get(side, fromPiece, move.to());
    if (historyScore != 0) {
        return stage * stageDiff - historyScore;
    }

    stage = 6;
    return stage * stageDiff;
}

std::optional<TMoveInfo> TSearchStrategy::MakeMove(
    const lczero::PositionHistory& history
) {
    auto timerStart = std::chrono::high_resolution_clock::now();
    size_t currentDepth = 0;
    HistoryHeuristics.Clear();
    TMoveInfo move;
    while (currentDepth <= Config.Depth) {
        TSearchNode startNode{
            history.Last(),
            currentDepth,
            0
        };
        move = Search(startNode, MIN_SCORE_VALUE, MAX_SCORE_VALUE);
        auto timerEnd = std::chrono::high_resolution_clock::now();
        move.NodesCount = startNode.TreeNodesCount;
        move.TimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            timerEnd - timerStart
        ).count();
        move.Depth = currentDepth;
        if (move.Score == MIN_SCORE_VALUE || move.Score == MAX_SCORE_VALUE) {
            break;
        }
        currentDepth += 1;
    }
    return move;
}
