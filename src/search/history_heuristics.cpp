#include "history_heuristics.h"

THistoryHeuristics::THistoryHeuristics() {
    Clear();
}

void THistoryHeuristics::Add(int side, EPieceType piece, const lczero::BoardSquare& square, int depth) {
    History[side][static_cast<size_t>(piece)][square.as_int()] += depth * depth;
}

int THistoryHeuristics::Get(int side, EPieceType piece, const lczero::BoardSquare& square) const {
    return History.at(side).at(static_cast<size_t>(piece)).at(square.as_int());
}

void THistoryHeuristics::Clear() {
    for (size_t i = 0; i < static_cast<size_t>(EPieceType::Count); i++) {
        History[0][i].fill(0);
        History[1][i].fill(0);
    }
}
