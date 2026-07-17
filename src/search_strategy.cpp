#include <search_strategy.h>
#include <evaluation.h>
#include <search/see.h>
#include <search/move_ordering.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

using ENodeType = TTranspositionTable::ENodeType;

// Mate scores are node-relative: mate in N plies from here = MAX_SCORE - N.
// Each backup step moves the mate one ply further, so faster mates win.
constexpr int MATE_SCORE_THRESHOLD = 90000;

// Pruning constants, tuned against the test suites and equal-time
// self-play. Root budgets below TSearchConfig::AggressiveDepthMin keep the
// original conservative pruning: exact tactics and full strength at the
// depths the EPD suites assert (custom_epd 5-11 hard, Bratko-Kopec at 11
// soft). Deeper budgets enable the aggressive stack (history-ordered
// quiets, log LMR, LMP, deep futility/RFP, IIR) that makes depth-12 search
// feasible; it beat the conservative search by ~80 Elo at 250 ms/move in
// self-play (116 games, 2026-07-17). Custom 15 (mate in four) is the most
// fragile custom_epd position - re-run custom_epd after any pruning
// change, and re-run a self-play match after any sizable one.
constexpr int RFP_MARGIN = 120;
constexpr int RFP_MAX_DEPTH = 8;
constexpr int FUTILITY_MARGIN = 100;
constexpr int FUTILITY_MAX_DEPTH = 8;
constexpr int LMP_BASE = 4;
constexpr int IIR_MIN_DEPTH = 4;
constexpr size_t LMR_MIN_MOVE_DEEP = 3;
constexpr int QSEARCH_DELTA_MARGIN = 200;
constexpr int QSEARCH_SEE_MARGIN = 80;
constexpr int ASPIRATION_WINDOW = 50;

// Log-scaled LMR reductions, growing with both depth and move number
static std::array<std::array<int8_t, 64>, 64> BuildLmrTable(double divisor) {
    std::array<std::array<int8_t, 64>, 64> table{};
    for (int d = 1; d < 64; ++d) {
        for (int m = 1; m < 64; ++m) {
            table[d][m] = static_cast<int8_t>(
                0.5 + std::log(d) * std::log(m) * 100.0 / divisor);
        }
    }
    return table;
}
static const auto LMR_TABLE = BuildLmrTable(150.0);

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
    int depth = node.Depth;
    const size_t ply = node.Ply;
    ENSURE(depth >= 0, "Incorrect depth for Search");

    // Root must always produce a move; aborted subtrees unwind with garbage
    // scores that MakeMove discards
    if (DeadlineEnabled && ply > 0) {
        if (Aborted) {
            return {alpha};
        }
        if (++NodesSinceTimeCheck >= 4096) {
            NodesSinceTimeCheck = 0;
            if (std::chrono::steady_clock::now() >= Deadline) {
                Aborted = true;
                return {alpha};
            }
        }
    }

    const uint64_t positionHash = node.HashValue;
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

    // With PST eval the move list is only needed for mate/stalemate
    // detection, so its generation is deferred: a valid TT move proves the
    // node is not terminal and often cuts off on its own
    lczero::MoveList ourLegalMoves;
    bool movesReady = false;
    if (!Config.EnablePST) {
        ourLegalMoves = board.GenerateLegalMoves();
        movesReady = true;
        if (IsTerminal(position, ourLegalMoves)) {
            return {Evaluate(position, ourLegalMoves, Config.EnablePST)};
        }
    } else if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
        return {0};
    }

    const bool isUnderCheck = node.IsUnderCheck();
    const bool deepMode = Config.Depth >= Config.AggressiveDepthMin;
    const bool isPvNode = beta - alpha > 1;

    // Internal iterative reduction: no TT move means the tree here was never
    // explored, a shallower search will fill the TT and be cheap to redo
    if (deepMode && Config.EnableTT && !ttEntry && depth >= IIR_MIN_DEPTH) {
        depth -= 1;
    }
    const bool nonMateWindow =
        alpha > -MATE_SCORE_THRESHOLD && beta < MATE_SCORE_THRESHOLD;
    const bool tryRFP = Config.EnableNullMove && !isUnderCheck
        && depth <= (deepMode ? RFP_MAX_DEPTH : 2) && nonMateWindow;
    const bool tryNullMove = Config.EnableNullMove && !isUnderCheck
        && depth >= Config.NullMoveDepthReduction + 1;
    const bool tryFutility = Config.EnableLMR && !isUnderCheck
        && depth <= (deepMode ? FUTILITY_MAX_DEPTH : 1) && nonMateWindow;
    const bool tryLMP = tryFutility && deepMode;
    const bool needStatic = tryRFP || tryNullMove || tryFutility;
    const int staticScore = !needStatic ? 0
        : (ttEntry && ttEntry->StaticEval != TTranspositionTable::UNKNOWN_EVAL)
            ? ttEntry->StaticEval
            : Config.EnablePST
                ? FinalizePstScore(node.Pst)
                : Evaluate(position, ourLegalMoves, Config.EnablePST);

    // Reverse futility: static eval is so far above beta that
    // no quiet reply is likely to bring it back down
    if (tryRFP && staticScore - RFP_MARGIN * depth >= beta) {
        return {staticScore};
    }

    // Null move; reduction grows with depth
    if (tryNullMove) {
        if (staticScore >= beta - Config.NullMoveEvalMargin) {
            const int reduction = Config.NullMoveDepthReduction
                + (deepMode ? depth / 4 : 0);
            const lczero::Position passPosition(
                position.GetThemBoard(), position.GetRule50Ply(), 0);
            TSearchNode nullNode(
                passPosition, node.Pst,
                std::max(0, depth - reduction - 1), ply + 1);
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
        // En passant can discover a check through the captured pawn's square
        const bool enPassant = board.pawns().get(move.from())
            && move.from().col() != move.to().col()
            && !board.theirs().get(move.to());
        return knightDistance || enPassant || alignedWithKing(move.from())
            || alignedWithKing(move.to()) || move.from() == ourKing;
    };

    // Move collection and ordering; a null move can be stored by qsearch
    // stand-pat entries and never stages
    const auto ttMove =
        (ttEntry && ttEntry->Depth >= 0 && ttEntry->Move.as_packed_int() != 0)
            ? std::make_optional(ttEntry->Move) : std::nullopt;
    const auto& prevMove = node.Move;
    // History ordering is part of the aggressive stack: it is what makes
    // the deeper reductions safe
    TMoveOrdering moveOrdering(
        position,
        prevMove,
        ttMove,
        Config.EnableKillers ? &KillerMoves : nullptr,
        (Config.EnableHH || deepMode) ? &HistoryHeuristics : nullptr
    );
    // Stage the TT move first: when it alone causes a cutoff, generating,
    // scoring and sorting the rest of the moves is wasted work. A deferred
    // (not yet generated) list means the TT move is trusted: the full hash
    // matched, so it was stored as legal for this very position.
    std::vector<TMoveInfo> moves;
    bool restPending = false;
    if (ttMove && (!movesReady
            || std::find(ourLegalMoves.begin(), ourLegalMoves.end(),
                   *ttMove) != ourLegalMoves.end())) {
        moves.emplace_back(*ttMove, 0);
        restPending = true;
    } else {
        if (!movesReady) {
            ourLegalMoves = board.GenerateLegalMoves();
            movesReady = true;
            if (ourLegalMoves.empty()) {
                return {isUnderCheck ? MIN_SCORE_VALUE : 0};
            }
        }
        moves = moveOrdering.Order(ourLegalMoves, ply);
    }

    // PVS negamax: first move with a full window, the rest with a zero
    // window and a re-search on fail-high
    size_t moveNumber = 0;
    TMoveInfo bestMoveInfo(MIN_SCORE_VALUE - 1);
    const size_t lmpThreshold = LMP_BASE + depth * depth;
    std::vector<lczero::Move> triedQuiets;
    for (size_t moveIdx = 0; ; ++moveIdx) {
        if (moveIdx == moves.size()) {
            if (!restPending) {
                break;
            }
            restPending = false;
            if (!movesReady) {
                ourLegalMoves = board.GenerateLegalMoves();
                movesReady = true;
            }
            lczero::MoveList rest;
            rest.reserve(ourLegalMoves.size() - 1);
            for (const auto& m : ourLegalMoves) {
                if (!(m == *ttMove)) {
                    rest.push_back(m);
                }
            }
            const auto ordered = moveOrdering.Order(rest, ply);
            moves.insert(moves.end(), ordered.begin(), ordered.end());
            if (moveIdx == moves.size()) {
                break;
            }
        }
        const auto& ourMove = moves[moveIdx].Move;
        const bool isCapture = IsCapture(position, ourMove);
        const bool isPromotion = IsPromotion(ourMove);
        const bool isQuiet = !isCapture && !isPromotion;

        // Late move pruning: quiet non-checking moves this late in a shallow
        // node are almost never best
        if (tryLMP && isQuiet && moveNumber >= lmpThreshold
                && !mayGiveCheck(ourMove)) {
            continue;
        }

        // Futility: quiet non-checking moves that can't raise alpha
        const bool futile = tryFutility && moveNumber > 0 && isQuiet
            && staticScore + FUTILITY_MARGIN * (depth + 1) <= alpha;
        if (futile && !mayGiveCheck(ourMove)) {
            continue;
        }

        TSearchNode child(node, ourMove, depth - 1, ply + 1);
        if (Config.EnableTT) {
            TranspositionTable.Prefetch(child.HashValue);
        }
        // Check extension: don't let forcing lines drop into qsearch early.
        // Ply cap bounds perpetual-check blowup. mayGiveCheck prefilters to
        // skip the expensive attack probe for moves that can't check, and
        // its verdict is cached on the child so it never re-probes.
        if (!mayGiveCheck(ourMove)) {
            child.InCheckFlag = 0;
        } else if (ply < 4 * static_cast<size_t>(Config.Depth)
                && child.IsUnderCheck()) {
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
            const size_t lmrMinMove = deepMode ? LMR_MIN_MOVE_DEEP
                : static_cast<size_t>(Config.LMRMinMoveNumber);
            const bool doLMR = Config.EnableLMR
                && moveNumber >= lmrMinMove
                && ply >= static_cast<size_t>(Config.LMRMinPly)
                && depth >= Config.LMRMinDepth
                && !isUnderCheck
                && childBaseDepth < depth // not a checking move
                && !isCapture
                && !isPromotion;
            if (doLMR) {
                int tableRed = LMR_TABLE[std::min(depth, 63)]
                                        [std::min<size_t>(moveNumber, 63)];
                if (deepMode) {
                    // PV nodes get one ply less reduction: exactness along
                    // the principal variation keeps tactics reliable.
                    // Moves the heuristics like are reduced less, moves
                    // that keep failing are reduced more.
                    tableRed -= isPvNode ? 1 : 0;
                    const int side = position.IsBlackToMove();
                    const int hist = HistoryHeuristics.Get(side, ourMove);
                    const auto& [k1, k2] = KillerMoves.GetMoves(ply);
                    const bool likelyGood =
                        (k1 && *k1 == ourMove) || (k2 && *k2 == ourMove)
                        || ourMove == HistoryHeuristics.GetCounterMove(node.Move)
                        || hist > 200;
                    if (likelyGood) {
                        tableRed -= 1;
                    } else if (hist < -200) {
                        tableRed += 1;
                    }
                }
                const int reduction = !deepMode
                    ? Config.LMRReduction
                    : std::max<int>(Config.LMRReduction, tableRed);
                child.Depth = std::max(0, childBaseDepth - reduction);
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
            if (isQuiet) {
                const int side = position.IsBlackToMove();
                const int bonus = std::min(depth * depth, 400);
                HistoryHeuristics.Add(side, ourMove, bonus);
                // Quiets tried before the cutoff move failed here: their
                // history decays so junk moves sink in the ordering
                for (const auto& tried : triedQuiets) {
                    HistoryHeuristics.Add(side, tried, -bonus);
                }
                HistoryHeuristics.AddCounterMove(node.Move, ourMove);
                KillerMoves.InsertMove(ourMove, ply);
            }
            bestMoveInfo = ourMoveInfo;
            break;
        }
        if (isQuiet) {
            triedQuiets.push_back(ourMove);
        }
        if (ourMoveInfo > bestMoveInfo) {
            bestMoveInfo = ourMoveInfo;
            alpha = std::max(alpha, ourScore);
        }
        moveNumber += 1;
    }

    if (Config.EnableTT && !Aborted) {
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
    const uint64_t positionHash = node.HashValue;
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
    } else if (!node.IsUnderCheck()) {
        // Not in check: mate is impossible here and stalemate goes
        // undetected (the standard qsearch tradeoff), so captures can be
        // generated directly and only when stand pat fails to cut off
        if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
            staticScore = 0;
            terminal = true;
        } else {
            staticScore =
                (ttEntry && ttEntry->StaticEval != TTranspositionTable::UNKNOWN_EVAL)
                    ? ttEntry->StaticEval
                    : FinalizePstScore(node.Pst);
            if (!terminal && !(Config.EnableAlphaBeta && staticScore >= beta)) {
                const auto pseudoCaptures = board.GenerateCaptures();
                if (!pseudoCaptures.empty()) {
                    const auto kingAttackInfo = board.GenerateKingAttackInfo();
                    for (const auto& move : pseudoCaptures) {
                        if (board.IsLegalMove(move, kingAttackInfo)) {
                            captureCandidates.push_back(move);
                        }
                    }
                }
            }
        }
    } else {
        // In check: full move generation to detect mate
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
            staticScore = MIN_SCORE_VALUE;
            terminal = true;
        } else if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
            staticScore = 0;
            terminal = true;
        } else {
            staticScore =
                (ttEntry && ttEntry->StaticEval != TTranspositionTable::UNKNOWN_EVAL)
                    ? ttEntry->StaticEval
                    : FinalizePstScore(node.Pst);
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
    std::vector<int> seeValues;
    for (const auto& move : captureCandidates) {
        EPieceType victim = GetPieceType(board, move.to());
        if (victim == EPieceType::Unknown) {
            victim = EPieceType::Pawn; // en passant
        }
        const int victimValue = GetPieceValue(victim);
        // Delta pruning: even winning this piece can't raise alpha
        if (Config.EnableAlphaBeta
            && staticScore + victimValue + QSEARCH_DELTA_MARGIN <= alpha) {
            continue;
        }
        // SEE is computed once here and reused for move ordering; a capture
        // of an equal or more valuable piece can't lose material, so only
        // cheaper-victim captures can be pruned by it
        const int attackerValue = GetPieceValue(GetPieceType(board, move.from()));
        const int seeValue = EvaluateCaptureSEE(position, move);
        if (victimValue < attackerValue && seeValue < 0) {
            continue;
        }
        // SEE futility: the exchange outcome itself can't raise alpha
        if (Config.EnableAlphaBeta
            && staticScore + seeValue + QSEARCH_SEE_MARGIN <= alpha) {
            continue;
        }
        filteredMoves.push_back(move);
        seeValues.push_back(seeValue);
    }
    if (filteredMoves.empty()) {
        return {staticScore};
    }
    const auto& captures = moveOrdering.Order(filteredMoves, ply, false, &seeValues);

    for (const auto& capture : captures) {
        const auto& ourMove = capture.Move;
        TSearchNode child(node, ourMove, depth - 1, ply + 1);
        if (Config.EnableTT) {
            TranspositionTable.Prefetch(child.HashValue);
        }
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
    DeadlineEnabled = Config.TimeLimitMs > 0;
    Aborted = false;
    NodesSinceTimeCheck = 0;
    if (DeadlineEnabled) {
        Deadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(Config.TimeLimitMs);
    }
    TMoveInfo move;
    bool haveCompletedIteration = false;
    int prevScore = 0;
    while (currentDepth <= Config.Depth) {
        TSearchNode startNode{
            history.Last(),
            currentDepth,
            0
        };
        int alpha = MIN_SCORE_VALUE;
        int beta = MAX_SCORE_VALUE;
        const bool useAspiration =
            Config.EnableAlphaBeta && Config.EnableTT && currentDepth >= 5;
        if (useAspiration) {
            alpha = prevScore - ASPIRATION_WINDOW;
            beta = prevScore + ASPIRATION_WINDOW;
        }
        TMoveInfo iterMove = Search(startNode, alpha, beta);
        if (!Aborted && useAspiration
                && (iterMove.Score <= alpha || iterMove.Score >= beta)) {
            iterMove = Search(startNode, MIN_SCORE_VALUE, MAX_SCORE_VALUE);
        }
        if (Aborted) {
            // Keep the last completed iteration's move; the aborted root
            // still returns a legal move as a fallback for the first one
            if (!haveCompletedIteration) {
                move = iterMove;
            }
            break;
        }
        move = iterMove;
        haveCompletedIteration = true;
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
        // Predictive stop: the next iteration typically costs at least as
        // much as everything so far, so starting it past half the budget
        // overshoots more often than it helps
        if (Config.TimeLimitMs > 0 && move.TimeMs * 2 >= Config.TimeLimitMs) {
            break;
        }
        currentDepth += 1;
    }
    return move;
}
