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
    // Saving variables
    const int alphaOrig = alpha;
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const int depth = node.Depth;
    const size_t ply = node.Ply;

    // TT lookup
    if (const auto ttNode = TranspositionTable.Find(position, depth)) {
        const auto& nodeType = ttNode->Type;
        const auto& nodeMove = ttNode->Move;
        if (nodeType == TTranspositionTable::ENodeType::PV) {
            return nodeMove;
        }
        if (nodeType == TTranspositionTable::ENodeType::Cut) {
            alpha = std::max(alpha, nodeMove.Score);
        } else if (nodeType == TTranspositionTable::ENodeType::All) {
            beta = std::min(beta, nodeMove.Score);
        }
        if (alpha >= beta) {
            return nodeMove;
        }
    }

    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

    // Eval for leaves
    const int staticScore = Evaluate(
        position, ourLegalMoves,
        theirLegalMoves, Config.EnablePST
    );

    if (IsTerminal(position, ourLegalMoves, theirLegalMoves)) {
        return {staticScore};
    }

    // Continue search in leaves for positions with captures
    if (depth == 0) {
        TSearchNode qNode(position, depth, ply);
        return QuiescenceSearch(qNode, alpha, beta);
    }

    // Move collection and ordering
    std::multimap<int, TSearchNode> children;
    for (const auto& move : ourLegalMoves) {
        int score = CalcMoveOrder(position, move, node.Move);
        children.emplace(score, TSearchNode(position, move, depth - 1, ply + 1));
    }

    // Alpha-beta negamax
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
            if (!IsCapture(position, ourMove) && !IsPromotion(ourMove)) {
                const int side = position.IsBlackToMove();
                EPieceType fromPiece = GetPieceType(board, ourMove.from());
                HistoryHeuristics.Add(side, fromPiece, ourMove, depth);
                HistoryHeuristics.AddCounterMove(node.Move, ourMove);
            }
            bestMoveInfo = ourMoveInfo;
            break;
        }
        if (ourMoveInfo > bestMoveInfo) {
            bestMoveInfo = ourMoveInfo;
            alpha = std::max(alpha, ourScore);
        }
    }

    // TT fill
    if (Config.EnableTT) {
        TTranspositionTable::ENodeType nodeType = TTranspositionTable::ENodeType::PV;
        if (bestMoveInfo.Score <= alphaOrig) {
            nodeType = TTranspositionTable::ENodeType::All;
        } else if (bestMoveInfo.Score >= beta) {
            nodeType = TTranspositionTable::ENodeType::Cut;
        }
        TranspositionTable.Insert(position, bestMoveInfo, depth, nodeType);
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
    const int depth = node.Depth;
    const size_t ply = node.Ply;

    const int staticScore = Evaluate(
        position, ourLegalMoves,
        theirLegalMoves, Config.EnablePST
    );

    if (IsTerminal(position, ourLegalMoves, theirLegalMoves) || (-depth == Config.QuiescenceSearchDepth)) {
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
    int currentDepth = 0;
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
