#include <search/move_ordering.h>

#include <search/see.h>


std::vector<TMoveInfo> TMoveOrdering::Order(
    const lczero::MoveList& moves,
    size_t ply,
    bool printScores /* = false */
) const {
    std::vector<TMoveInfo> movesScores;
    movesScores.reserve(moves.size());
    for (const lczero::Move& move : moves) {
        movesScores.emplace_back(move, CalcMoveOrder(move, ply));
    }
    std::stable_sort(movesScores.begin(), movesScores.end(),
        [](const TMoveInfo & a, const TMoveInfo& b) -> bool {
            if (a.Score < b.Score) return true;
            if (a.Score > b.Score) return false;
            return (a.Move.as_packed_int() < b.Move.as_packed_int());
        }
    );
    if (printScores) {
        for (const auto& move : movesScores) {
            std::cerr << move.Move.as_string() << "(" << move.Score << ") ";
        }
        std::cerr << std::endl;
    }
    return movesScores;
}

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
