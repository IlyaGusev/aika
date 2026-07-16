#include <search_strategy.h>
#include <evaluation.h>
#include <search/see.h>
#include <search/move_ordering.h>

#include <algorithm>
#include <chrono>

using ENodeType = TTranspositionTable::ENodeType;

// Mate scores are node-relative: mate in N plies from here = MAX_SCORE - N.
// Each backup step moves the mate one ply further, so faster mates win.
constexpr int MATE_SCORE_THRESHOLD = 90000;

static int AdjustChildScore(int score) {
    if (score > MATE_SCORE_THRESHOLD) {
        return score - 1;
    }
    if (score < -MATE_SCORE_THRESHOLD) {
        return score + 1;
    }
    return score;
}

TMoveInfo TSearchStrategy::Search(
    const TSearchNode& node,
    int alpha,
    int beta
) {
    const int alphaOrig = alpha;
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const int depth = node.Depth;
    const size_t ply = node.Ply;
    ENSURE(depth >= 0, "Incorrect depth for Search");

    const uint64_t positionHash = Config.EnableTT ? board.Hash() : 0;
    const auto* ttEntry = Config.EnableTT ? TranspositionTable.Find(positionHash) : nullptr;
    if (ttEntry && ttEntry->Depth >= depth) {
        if (ttEntry->Type == ENodeType::PV) {
            return {ttEntry->Move, ttEntry->Score};
        } else if (ttEntry->Type == ENodeType::Cut) {
            alpha = std::max(alpha, ttEntry->Score);
        } else if (ttEntry->Type == ENodeType::All) {
            beta = std::min(beta, ttEntry->Score);
        }
        if (alpha >= beta) {
            return {ttEntry->Move, ttEntry->Score};
        }
    }

    // Continue search in leaves for positions with captures; qsearch does
    // its own cheaper move generation and terminal detection
    if (depth == 0) {
        return QuiescenceSearch(node, alpha, beta);
    }

    const auto& ourLegalMoves = board.GenerateLegalMoves();

    if (IsTerminal(position, ourLegalMoves)) {
        return {Evaluate(position, ourLegalMoves, Config.EnablePST)};
    }

    const bool isUnderCheck = board.IsUnderCheck();

    constexpr int RFP_MARGIN = 120;
    constexpr int FUTILITY_MARGIN = 250;
    const bool nonMateWindow =
        alpha > -MATE_SCORE_THRESHOLD && beta < MATE_SCORE_THRESHOLD;
    const bool tryRFP = Config.EnableNullMove && !isUnderCheck
        && depth <= 2 && nonMateWindow;
    const bool tryNullMove = Config.EnableNullMove && !isUnderCheck
        && depth >= Config.NullMoveDepthReduction + 1;
    const bool tryFutility = Config.EnableLMR && !isUnderCheck
        && depth == 1 && nonMateWindow;
    const bool needStatic = tryRFP || tryNullMove || tryFutility;
    const int staticScore = !needStatic ? 0
        : (ttEntry && ttEntry->StaticEval != TTranspositionTable::UNKNOWN_EVAL)
            ? ttEntry->StaticEval
            : Evaluate(position, ourLegalMoves, Config.EnablePST);

    // Reverse futility: static eval is so far above beta that
    // no quiet reply is likely to bring it back down
    if (tryRFP && staticScore - RFP_MARGIN * depth >= beta) {
        return {staticScore};
    }

    // Null move
    if (tryNullMove) {
        if (staticScore >= beta - Config.NullMoveEvalMargin) {
            const lczero::Position passPosition(
                position.GetThemBoard(), position.GetRule50Ply(), 0);
            TSearchNode nullNode(
                passPosition, depth - Config.NullMoveDepthReduction - 1, ply + 1);
            const int score = AdjustChildScore(-Search(nullNode, -beta, -beta + 1).Score);
            node.TreeNodesCount += nullNode.TreeNodesCount;
            if (score >= beta) {
                return TMoveInfo{lczero::Move(), beta};
            }
        }
    }

    // Conservative "may give check" test for futility: from/to aligned with
    // the enemy king (direct or discovered check), knight distance, or any
    // king move (castling rook checks). False only when a check is impossible.
    const auto theirKing = *(board.kings() & board.theirs()).begin();
    const auto ourKing = *(board.kings() & board.ours()).begin();
    const auto mayGiveCheck = [&](const lczero::Move& move) {
        const auto alignedWithKing = [&](const lczero::BoardSquare& sq) {
            const int dr = sq.row() - theirKing.row();
            const int dc = sq.col() - theirKing.col();
            return dr == 0 || dc == 0 || dr == dc || dr == -dc;
        };
        const int dr = std::abs(move.to().row() - theirKing.row());
        const int dc = std::abs(move.to().col() - theirKing.col());
        const bool knightDistance = (dr == 1 && dc == 2) || (dr == 2 && dc == 1);
        return knightDistance || alignedWithKing(move.from())
            || alignedWithKing(move.to()) || move.from() == ourKing;
    };

    // Move collection and ordering
    const auto ttMove = (ttEntry && ttEntry->Depth >= 0)
        ? std::make_optional(ttEntry->Move) : std::nullopt;
    const auto& prevMove = node.Move;
    TMoveOrdering moveOrdering(
        position,
        prevMove,
        ttMove,
        Config.EnableKillers ? &KillerMoves : nullptr,
        Config.EnableHH ? &HistoryHeuristics : nullptr
    );
    const auto& moves = moveOrdering.Order(ourLegalMoves, ply);

    // PVS negamax: first move with a full window, the rest with a zero
    // window and a re-search on fail-high
    size_t moveNumber = 0;
    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    for (const auto& moveInfo : moves) {
        const auto& ourMove = moveInfo.Move;
        const bool isCapture = IsCapture(position, ourMove);
        const bool isPromotion = IsPromotion(ourMove);

        // Futility: quiet non-checking frontier moves that can't raise alpha
        const bool futile = tryFutility && moveNumber > 0
            && !isCapture && !isPromotion
            && staticScore + FUTILITY_MARGIN <= alpha;
        if (futile && !mayGiveCheck(ourMove)) {
            continue;
        }

        TSearchNode child(position, ourMove, depth - 1, ply + 1);
        // Check extension: don't let forcing lines drop into qsearch early.
        // Ply cap bounds perpetual-check blowup.
        if (ply < 4 * static_cast<size_t>(Config.Depth)
                && child.Position.GetBoard().IsUnderCheck()) {
            child.Depth = depth;
        }
        const int childBaseDepth = child.Depth;
        if (futile && childBaseDepth == 0) {
            continue;
        }

        int ourScore;
        if (moveNumber == 0 || !Config.EnableAlphaBeta) {
            ourScore = AdjustChildScore(-Search(child, -beta, -alpha).Score);
        } else {
            const bool doLMR = Config.EnableLMR
                && moveNumber >= static_cast<size_t>(Config.LMRMinMoveNumber)
                && ply >= static_cast<size_t>(Config.LMRMinPly)
                && depth >= Config.LMRMinDepth
                && !isUnderCheck
                && childBaseDepth < depth // not a checking move
                && !isCapture
                && !isPromotion;
            if (doLMR) {
                child.Depth = childBaseDepth - Config.LMRReduction;
                ourScore = AdjustChildScore(-Search(child, -alpha - 1, -alpha).Score);
                if (ourScore > alpha) {
                    child.Depth = childBaseDepth;
                    ourScore = AdjustChildScore(-Search(child, -alpha - 1, -alpha).Score);
                }
            } else {
                ourScore = AdjustChildScore(-Search(child, -alpha - 1, -alpha).Score);
            }
            if (ourScore > alpha && ourScore < beta) {
                child.Depth = childBaseDepth;
                ourScore = AdjustChildScore(-Search(child, -beta, -alpha).Score);
            }
        }
        node.TreeNodesCount += child.TreeNodesCount;

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

    if (Config.EnableTT) {
        const int32_t evalToStore = needStatic ? staticScore
            : ttEntry ? ttEntry->StaticEval : TTranspositionTable::UNKNOWN_EVAL;
        TranspositionTable.Insert(
            positionHash, bestMoveInfo, depth, alphaOrig, beta, evalToStore);
    }
    return bestMoveInfo;
}

TMoveInfo TSearchStrategy::QuiescenceSearch(
    const TSearchNode& node,
    int alpha,
    int beta
) {
    const int alphaOrig = alpha;
    const auto& position = node.Position;
    const auto& board = position.GetBoard();
    const int depth = node.Depth;
    const size_t ply = node.Ply;
    ENSURE(depth <= 0, "Incorrect depth for QSearch");

    // Probe before movegen: only non-terminal positions are ever inserted
    const uint64_t positionHash = Config.EnableTT ? board.Hash() : 0;
    const auto* ttEntry = Config.EnableTT ? TranspositionTable.Find(positionHash) : nullptr;
    if (ttEntry && ttEntry->Depth >= depth) {
        if (ttEntry->Type == ENodeType::PV) {
            return {ttEntry->Move, ttEntry->Score};
        } else if (ttEntry->Type == ENodeType::Cut) {
            alpha = std::max(alpha, ttEntry->Score);
        } else if (ttEntry->Type == ENodeType::All) {
            beta = std::min(beta, ttEntry->Score);
        }
        if (Config.EnableAlphaBeta && alpha >= beta) {
            return {ttEntry->Move, ttEntry->Score};
        }
    }

    lczero::MoveList captureCandidates;
    int staticScore;
    bool terminal = (-depth == Config.QuiescenceSearchDepth);
    if (!Config.EnablePST) {
        const auto& ourLegalMoves = board.GenerateLegalMoves();
        staticScore = Evaluate(position, ourLegalMoves, Config.EnablePST);
        terminal = terminal || IsTerminal(position, ourLegalMoves);
        for (const auto& move : ourLegalMoves) {
            if (IsCapture(position, move)) {
                captureCandidates.push_back(move);
            }
        }
    } else {
        // Generating all legal moves is wasteful when only captures are
        // searched: legality-check captures only, plus the first legal
        // move to detect mate/stalemate
        const auto pseudoMoves = board.GeneratePseudolegalMoves();
        const auto kingAttackInfo = board.GenerateKingAttackInfo();
        bool anyLegal = false;
        for (const auto& move : pseudoMoves) {
            const bool isCapture = IsCapture(position, move);
            if ((isCapture || !anyLegal) && board.IsLegalMove(move, kingAttackInfo)) {
                anyLegal = true;
                if (isCapture) {
                    captureCandidates.push_back(move);
                }
            }
        }
        if (!anyLegal) {
            staticScore = board.IsUnderCheck() ? MIN_SCORE_VALUE : 0;
            terminal = true;
        } else if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
            staticScore = 0;
            terminal = true;
        } else {
            staticScore =
                (ttEntry && ttEntry->StaticEval != TTranspositionTable::UNKNOWN_EVAL)
                    ? ttEntry->StaticEval
                    : CalcPSTScore(position);
        }
    }

    if (terminal) {
        return {staticScore};
    }

    if (Config.EnableAlphaBeta && staticScore >= beta) {
        return {staticScore};
    }
    alpha = std::max(alpha, staticScore);

    const auto ttMove = (ttEntry && ttEntry->Depth >= 0)
        ? std::make_optional(ttEntry->Move) : std::nullopt;
    TMoveOrdering moveOrdering(
        position,
        node.Move,
        ttMove,
        Config.EnableKillers ? &KillerMoves : nullptr,
        Config.EnableHH ? &HistoryHeuristics : nullptr
    );
    // Fail-soft stand pat: the side to move may decline all captures
    TMoveInfo bestMoveInfo(staticScore);
    std::vector<lczero::Move> filteredMoves;
    for (const auto& move : captureCandidates) {
        EPieceType victim = GetPieceType(board, move.to());
        if (victim == EPieceType::Unknown) {
            victim = EPieceType::Pawn; // en passant
        }
        const int victimValue = GetPieceValue(victim);
        // Delta pruning: even winning this piece can't raise alpha
        constexpr int DELTA_PRUNING_MARGIN = 200;
        if (Config.EnableAlphaBeta
            && staticScore + victimValue + DELTA_PRUNING_MARGIN <= alpha) {
            continue;
        }
        const int attackerValue = GetPieceValue(GetPieceType(board, move.from()));
        // A capture of an equal or more valuable piece can't lose material;
        // run full SEE only for the rest
        if (victimValue < attackerValue && EvaluateCaptureSEE(position, move) < 0) {
            continue;
        }
        filteredMoves.push_back(move);
    }
    if (filteredMoves.empty()) {
        return {staticScore};
    }
    const auto& captures = moveOrdering.Order(filteredMoves, ply);

    for (const auto& capture : captures) {
        const auto& ourMove = capture.Move;
        TSearchNode child(position, ourMove, depth - 1, ply + 1);
        const int ourScore = AdjustChildScore(-QuiescenceSearch(child, -beta, -alpha).Score);
        node.TreeNodesCount += child.TreeNodesCount;

        if (Config.EnableAlphaBeta && ourScore >= beta) {
            bestMoveInfo = {ourMove, beta};
            break;
        }
        if (ourScore > bestMoveInfo.Score) {
            bestMoveInfo = {ourMove, ourScore};
            alpha = std::max(alpha, ourScore);
        }
    }

    if (Config.EnableTT) {
        TranspositionTable.Insert(
            positionHash, bestMoveInfo, depth, alphaOrig, beta, staticScore);
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
    int prevScore = 0;
    while (currentDepth <= Config.Depth) {
        TSearchNode startNode{
            history.Last(),
            currentDepth,
            0
        };
        constexpr int ASPIRATION_WINDOW = 50;
        int alpha = MIN_SCORE_VALUE;
        int beta = MAX_SCORE_VALUE;
        const bool useAspiration =
            Config.EnableAlphaBeta && Config.EnableTT && currentDepth >= 5;
        if (useAspiration) {
            alpha = prevScore - ASPIRATION_WINDOW;
            beta = prevScore + ASPIRATION_WINDOW;
        }
        move = Search(startNode, alpha, beta);
        if (useAspiration && (move.Score <= alpha || move.Score >= beta)) {
            move = Search(startNode, MIN_SCORE_VALUE, MAX_SCORE_VALUE);
        }
        auto timerEnd = std::chrono::high_resolution_clock::now();
        move.NodesCount = startNode.TreeNodesCount;
        move.TimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            timerEnd - timerStart
        ).count();
        move.Depth = currentDepth;
        prevScore = move.Score;
        if (move.Score == MIN_SCORE_VALUE || move.Score == MAX_SCORE_VALUE) {
            break;
        }
        currentDepth += 1;
    }
    return move;
}
