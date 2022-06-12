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
    const int alphaOrig = alpha;
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const size_t depth = node.Depth;
    const size_t ply = node.Ply;

    const auto ttNode = TranspositionTable.Find(
        position,
        depth
    );
    if (ttNode) {
        const auto& nodeType = ttNode->Type;
        const auto& nodeMove = ttNode->Move;
        if (nodeType == TTranspositionTable::ENodeType::PV) {
            return nodeMove;
        } else if (nodeType == TTranspositionTable::ENodeType::Cut && nodeMove.Score >= beta) {
            return nodeMove;
        } else if (nodeType == TTranspositionTable::ENodeType::All && nodeMove.Score <= alpha) {
            return nodeMove;
        }
    }

    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

    const int staticScore = Evaluate(
        position, ourLegalMoves,
        theirLegalMoves, Config.EnablePST
    );

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
        int score = CalcMoveOrder(position, move, node.Move);
        children.emplace(score, TSearchNode(position, move, depth - 1, ply + 1));
    }

    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (auto& [_, child] : children) {
        UNUSED(_);
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
            if (!IsCapture(position, ourMove) && !IsPromotion(ourMove)) {
                int side = position.IsBlackToMove();
                EPieceType fromPiece = GetPieceType(board, ourMove.from());
                HistoryHeuristics.Add(side, fromPiece, ourMove, depth);
                HistoryHeuristics.AddCounterMove(node.Move, ourMove);
            }
            return ourMoveInfo;
        }
        if (ourMoveInfo > bestMoveInfo) {
            bestMoveInfo = ourMoveInfo;
            alpha = std::max(alpha, ourScore);
        }
    }
    if (Config.EnableTT) {
        if (bestMoveInfo.Score <= alphaOrig) {
            TranspositionTable.Insert(position, bestMoveInfo, depth,
                TTranspositionTable::ENodeType::All
            );
        } else {
            TranspositionTable.Insert(position, bestMoveInfo, depth,
                TTranspositionTable::ENodeType::PV
            );
        }
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
            int score = CalcMoveOrder(position, move, node.Move);
            children.emplace(score, TSearchNode(position, move, depth - 1, ply + 1));
        }
    }

    if (children.empty()) {
        return {staticScore};
    }

    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (auto& [_, child] : children) {
        UNUSED(_);
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
    const lczero::Move& prevMove
) const {
    int score = 0;

    // Hash move
    std::optional<TTranspositionTable::TNode> bm = TranspositionTable.Find(position);
    if (bm && bm->Move.Move == move) {
        score += 10000;
        return -score;
    }

    // Promotions
    if (IsPromotion(move)) {
        score += 2000 + GetPieceValue(EPieceType::Queen);
    }

    // Captures
    if (IsCapture(position, move)) {
        int captureValue = EvaluateCaptureSEE(position, move);
        score += 2000 + captureValue;
        return -score;
    }

    // History heuristics
    EPieceType fromPiece = GetPieceType(position.GetBoard(), move.from());
    int side = position.IsBlackToMove();
    int historyScore = HistoryHeuristics.Get(side, fromPiece, move);
    if (historyScore != 0) {
        score += 500 + historyScore;
    }

    // Counter moves
    auto counterMove = HistoryHeuristics.GetCounterMove(prevMove);
    if (move == counterMove) {
        score += 100;
    }

    return -score;
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
