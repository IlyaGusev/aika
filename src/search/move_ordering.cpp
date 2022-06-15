#include <search/move_ordering.h>

#include <search/see.h>

int TMoveOrdering::CalcMoveOrder(const lczero::Move& move, size_t ply) const {
    int score = 0;

    // Hash move
    if (TTMove && *TTMove == move) {
        score += 10000;
        return -score;
    }

    // Promotions
    if (IsPromotion(move)) {
        score += 2000 + GetPieceValue(EPieceType::Queen);
    }

    // Captures
    if (IsCapture(Position, move)) {
        int captureValue = EvaluateCaptureSEE(Position, move);
        score += 2000 + captureValue;
    }

    if (KillerMoves) {
        const auto& [firstKiller, secondKiller] = KillerMoves->GetMoves(ply);
        if (firstKiller && *firstKiller == move) {
            score += 1000;
        }
        if (secondKiller && *secondKiller == move) {
            score += 999;
        }
    }

    if (score != 0) {
        return -score;
    }

    // History heuristics and counter moves
    if (HistoryHeuristics) {
        EPieceType fromPiece = GetPieceType(Position.GetBoard(), move.from());
        int side = Position.IsBlackToMove();
        int historyScore = HistoryHeuristics->Get(side, fromPiece, move);
        if (historyScore != 0) {
            score += 500 + historyScore;
        }

        auto counterMove = HistoryHeuristics->GetCounterMove(PrevMove);
        if (move == counterMove) {
            score += 100;
        }
    }

    return -score;
}
