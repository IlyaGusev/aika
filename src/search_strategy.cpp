#include <search_strategy.h>
#include <evaluation.h>
#include <search/see.h>
#include <search/move_ordering.h>

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
    const bool isUnderCheck = board.IsUnderCheck();
    ENSURE(depth >= 0, "Incorrect depth for Search");

    // TT lookup
    if (const auto ttNode = TranspositionTable.Find(position, depth)) {
        const auto& nodeType = ttNode->Type;
        const auto& nodeMove = ttNode->Move;
        if (nodeType == TTranspositionTable::ENodeType::PV) {
            return nodeMove;
        } else if (nodeType == TTranspositionTable::ENodeType::Cut) {
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

    // Null move
    if (Config.EnableNullMove) {
        const size_t reduction = Config.NullMoveDepthReduction;
        const size_t margin = Config.NullMoveEvalMargin;
        const auto nullMove = TryNullMove(
            node, beta, staticScore, reduction, margin
        );
        if (nullMove) {
            return *nullMove;
        }
    }

    // Move collection and ordering
    const auto ttNode = TranspositionTable.Find(position);
    const auto ttMove = ttNode ? std::make_optional(ttNode->Move.Move) : std::nullopt;
    const auto& prevMove = node.Move;
    TMoveOrdering moveOrdering(
        position,
        prevMove,
        ttMove,
        Config.EnableHH ? &HistoryHeuristics : nullptr
    );
    const auto& moves = moveOrdering.Order(ourLegalMoves);

    // Alpha-beta negamax
    size_t moveNumber = 0;
    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (const auto& moveInfo : moves) {
        const auto& ourMove = moveInfo.Move;
        const bool isCapture = IsCapture(position, ourMove);
        const bool isPromotion = IsPromotion(ourMove);

        int ourScore = 0;
        bool lmrConditions = true
            && moveNumber >= 5
            && ply >= 2
            && !isCapture
            && !isPromotion
            && !isUnderCheck
            && depth >= 2;
        if (Config.EnableLMR && lmrConditions) {
            TSearchNode lmrChild(position, ourMove, depth - 2, ply + 1);
            ourScore = -Search(lmrChild, -beta, -alpha).Score;
            node.TreeNodesCount += lmrChild.TreeNodesCount;
            if(ourScore >= alpha) {
                TSearchNode child(position, ourMove, depth - 1, ply + 1);
                ourScore = -Search(child, -beta, -alpha).Score;
                node.TreeNodesCount += child.TreeNodesCount;
            }
        } else {
            TSearchNode child(position, ourMove, depth - 1, ply + 1);
            ourScore = -Search(child, -beta, -alpha).Score;
            node.TreeNodesCount += child.TreeNodesCount;
        }

        TMoveInfo ourMoveInfo(ourMove, ourScore);
        if (Config.EnableAlphaBeta && ourScore >= beta) {
            if (!isCapture && !isPromotion) {
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
        moveNumber += 1;
    }

    // TT fill
    if (Config.EnableTT) {
        TranspositionTable.Insert(position, bestMoveInfo, depth, alphaOrig, beta);
    }
    return bestMoveInfo;
}

std::optional<TMoveInfo> TSearchStrategy::TryNullMove(
    const TSearchNode& node,
    int beta,
    int staticScore,
    size_t depthReduction,
    int evalMargin
) {
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const auto& theirBoard = position.GetThemBoard();
    const int depth = node.Depth;
    const size_t ply = node.Ply;
    const bool isUnderCheck = board.IsUnderCheck();

    if (!isUnderCheck && depth >= depthReduction + 1 && staticScore >= beta - evalMargin) {
        const auto& newPosition = lczero::Position(theirBoard, position.GetRule50Ply(), 0);
        TSearchNode searchNode(newPosition, lczero::Move(), depth - depthReduction - 1, ply + 1);
        const int score = -Search(searchNode, -beta, -beta + 1).Score;
        node.TreeNodesCount += searchNode.TreeNodesCount;
        if (score >= beta) {
            return TMoveInfo{lczero::Move(), beta};
        }
    }
    return std::nullopt;
}

TMoveInfo TSearchStrategy::QuiescenceSearch(
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
    ENSURE(depth <= 0, "Incorrect depth for QSearch");

    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

    const int staticScore = Evaluate(
        position, ourLegalMoves,
        theirLegalMoves, Config.EnablePST
    );

    if (IsTerminal(position, ourLegalMoves, theirLegalMoves) || (-depth == Config.QuiescenceSearchDepth)) {
        return {staticScore};
    }
    alpha = std::max(alpha, staticScore);

    // TT lookup
    if (const auto ttNode = TranspositionTable.Find(position, depth)) {
        const auto& nodeType = ttNode->Type;
        const auto& nodeMove = ttNode->Move;
        if (nodeType == TTranspositionTable::ENodeType::PV) {
            return nodeMove;
        } else if (nodeType == TTranspositionTable::ENodeType::Cut) {
            alpha = std::max(alpha, nodeMove.Score);
        } else if (nodeType == TTranspositionTable::ENodeType::All) {
            beta = std::min(beta, nodeMove.Score);
        }
    }

    if (Config.EnableAlphaBeta && alpha >= beta) {
        return {beta};
    }

    const auto ttNode = TranspositionTable.Find(position);
    const auto ttMove = ttNode ? std::make_optional(ttNode->Move.Move) : std::nullopt;
    const auto& prevMove = node.Move;
    TMoveOrdering moveOrdering(
        position,
        prevMove,
        ttMove,
        Config.EnableHH ? &HistoryHeuristics : nullptr
    );
    std::vector<lczero::Move> filteredMoves;
    filteredMoves.reserve(filteredMoves.size());
    for (const auto& move : ourLegalMoves) {
        if (IsCapture(position, move) && EvaluateCaptureSEE(position, move) >= 0) {
            filteredMoves.push_back(move);
        }
    }
    if (filteredMoves.empty()) {
        return {staticScore};
    }

    const auto& moves = moveOrdering.Order(filteredMoves);

    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (const auto& move : moves) {
        const auto& ourMove = move.Move;

        TSearchNode child(position, ourMove, depth - 1, ply + 1);
        const auto& bestEnemyMove = QuiescenceSearch(child, -beta, -alpha);
        const int bestEnemyScore = bestEnemyMove.Score;
        node.TreeNodesCount += child.TreeNodesCount;

        const int ourScore = -bestEnemyScore;
        TMoveInfo ourMoveInfo(ourMove, ourScore);

        if (Config.EnableAlphaBeta && ourScore >= beta) {
            bestMoveInfo = {ourMove, beta};
            break;
        }
        if (ourMoveInfo > bestMoveInfo) {
            bestMoveInfo = ourMoveInfo;
            alpha = std::max(alpha, ourScore);
        }
    }

    // TT fill
    if (Config.EnableTT) {
        TranspositionTable.Insert(position, bestMoveInfo, depth, alphaOrig, beta);
    }
    return bestMoveInfo;
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
