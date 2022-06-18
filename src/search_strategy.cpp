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
        const auto nullMove = TryNullMove(node, beta, staticScore);
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
        Config.EnableKillers ? &KillerMoves : nullptr,
        Config.EnableHH ? &HistoryHeuristics : nullptr
    );
    const auto& moves = moveOrdering.Order(ourLegalMoves, ply);

    // Alpha-beta negamax
    size_t moveNumber = 0;
    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (const auto& moveInfo : moves) {
        const auto& ourMove = moveInfo.Move;
        const bool isCapture = IsCapture(position, ourMove);
        const bool isPromotion = IsPromotion(ourMove);

        std::optional<int> optScore = std::nullopt;
        if (Config.EnableLMR) {
            const auto lmrMoveInfo = TryLMR(node, ourMove, alpha, beta, moveNumber);
            if (lmrMoveInfo) {
                optScore = lmrMoveInfo->Score;
            }
        }
        if (!optScore) {
            TSearchNode child(position, ourMove, depth - 1, ply + 1);
            optScore = -Search(child, -beta, -alpha).Score;
            node.TreeNodesCount += child.TreeNodesCount;
        }

        const int ourScore = *optScore;
        TMoveInfo ourMoveInfo(ourMove, ourScore);
        if (Config.EnableAlphaBeta && ourScore >= beta) {
            if (!isCapture && !isPromotion) {
                const int side = position.IsBlackToMove();
                EPieceType fromPiece = GetPieceType(board, ourMove.from());
                HistoryHeuristics.Add(side, fromPiece, ourMove, depth);
                HistoryHeuristics.AddCounterMove(node.Move, ourMove);
                KillerMoves.InsertMove(ourMove, ply);
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
    int staticScore
) {
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const auto& theirBoard = position.GetThemBoard();
    const int depth = node.Depth;
    const size_t ply = node.Ply;
    const bool isUnderCheck = board.IsUnderCheck();

    const size_t reduction = Config.NullMoveDepthReduction;
    const int margin = Config.NullMoveEvalMargin;
    if (!isUnderCheck && depth >= reduction + 1 && staticScore >= beta - margin) {
        const auto& newPosition = lczero::Position(theirBoard, position.GetRule50Ply(), 0);
        TSearchNode searchNode(newPosition, lczero::Move(), depth - reduction - 1, ply + 1);
        const int score = -Search(searchNode, -beta, -beta + 1).Score;
        node.TreeNodesCount += searchNode.TreeNodesCount;
        if (score >= beta) {
            return TMoveInfo{lczero::Move(), beta};
        }
    }
    return std::nullopt;
}

std::optional<TMoveInfo> TSearchStrategy::TryLMR(
    const TSearchNode& node,
    const lczero::Move& move,
    int alpha,
    int beta,
    size_t moveNumber
) {
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const int depth = node.Depth;
    const size_t ply = node.Ply;
    const bool isUnderCheck = board.IsUnderCheck();
    const bool isCapture = IsCapture(position, move);
    const bool isPromotion = IsPromotion(move);

    int score = 0;
    bool lmrConditions = true
        && moveNumber >= Config.LMRMinMoveNumber
        && ply >= Config.LMRMinPly
        && !isCapture
        && !isPromotion
        && !isUnderCheck
        && depth >= Config.LMRMinDepth;
    if (!lmrConditions) {
        return std::nullopt;
    }
    TSearchNode lmrChild(position, move, depth - 2, ply + 1);
    score = -Search(lmrChild, -beta, -alpha).Score;
    node.TreeNodesCount += lmrChild.TreeNodesCount;
    if (score >= alpha) {
        TSearchNode child(position, move, depth - 1, ply + 1);
        score = -Search(child, -beta, -alpha).Score;
        node.TreeNodesCount += child.TreeNodesCount;
    }
    return TMoveInfo{move, score};
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
        Config.EnableKillers ? &KillerMoves : nullptr,
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

    const auto& moves = moveOrdering.Order(filteredMoves, ply);

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
    PRINT_DEBUG(TranspositionTable.GetStats());
    return move;
}
