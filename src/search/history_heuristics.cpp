#include <search/history_heuristics.h>

THistoryHeuristics::THistoryHeuristics() {
    Clear();
}

void THistoryHeuristics::Add(
    int side,
    EPieceType piece,
    const lczero::Move& move,
    int depth
) {
    const size_t pieceType = static_cast<size_t>(piece);
    PieceHistory[side][pieceType][move.to().as_int()] += depth * depth;
}

int THistoryHeuristics::Get(
    int side,
    EPieceType piece,
    const lczero::Move& move
) const {
    const size_t pieceType = static_cast<size_t>(piece);
    const int pieceScore = PieceHistory.at(side).at(pieceType).at(move.to().as_int());
    return pieceScore;
}

void THistoryHeuristics::AddCounterMove(
    const lczero::Move& previousMove,
    const lczero::Move& move
) {
    CounterMoves[previousMove.from().as_int()][previousMove.to().as_int()] = move;
}

lczero::Move THistoryHeuristics::GetCounterMove(const lczero::Move& previousMove) const {
    return CounterMoves.at(previousMove.from().as_int()).at(previousMove.to().as_int());
}

void THistoryHeuristics::Clear() {
    for (size_t i = 0; i < static_cast<size_t>(EPieceType::Count); i++) {
        PieceHistory[0][i].fill(0);
        PieceHistory[1][i].fill(0);
    }
    for (size_t i = 0; i < 64; i++) {
        CounterMoves[i].fill(lczero::Move());
    }
}
