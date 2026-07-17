#include <search/move_ordering.h>

#include <search/see.h>


std::vector<TMoveInfo> TMoveOrdering::Order(
    const lczero::MoveList& moves,
    size_t ply,
    bool printScores /* = false */,
    const std::vector<int>* seeValues /* = nullptr */
) const {
    std::vector<TMoveInfo> movesScores;
    movesScores.reserve(moves.size());
    // Composite key: order score in the high bits, packed move as the tie
    // break, so the sort comparator is a single integer compare
    std::vector<std::pair<int64_t, size_t>> keys;
    keys.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); ++i) {
        const lczero::Move& move = moves[i];
        const int64_t key =
            (static_cast<int64_t>(CalcMoveOrder(
                move, ply, seeValues ? &(*seeValues)[i] : nullptr)) << 16)
            | move.as_packed_int();
        keys.emplace_back(key, i);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& [key, i] : keys) {
        movesScores.emplace_back(
            moves[i], static_cast<int>(key >> 16));
    }
    if (printScores) {
        for (const auto& move : movesScores) {
            std::cerr << move.Move.as_string() << "(" << move.Score << ") ";
        }
        std::cerr << std::endl;
    }
    return movesScores;
}

int TMoveOrdering::CalcMoveOrder(
    const lczero::Move& move, size_t ply, const int* seeValue) const {
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
        const int captureValue =
            seeValue ? *seeValue : EvaluateCaptureSEE(Position, move);
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

    // Quiets are ordered by their bounded history score (below killers),
    // with a bonus for the counter move to the previous move
    if (HistoryHeuristics) {
        const int side = Position.IsBlackToMove();
        score += HistoryHeuristics->Get(side, move);
        if (move == HistoryHeuristics->GetCounterMove(PrevMove)) {
            score += 300;
        }
    }

    return -score;
}
