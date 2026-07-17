#include <search/history_heuristics.h>

#include <algorithm>
#include <cstdlib>

THistoryHeuristics::THistoryHeuristics() {
    Clear();
}

void THistoryHeuristics::Add(int side, const lczero::Move& move, int bonus) {
    bonus = std::clamp(bonus, -HISTORY_MAX, HISTORY_MAX);
    int& value = FromToHistory[side][move.from().as_int()][move.to().as_int()];
    value += bonus - value * std::abs(bonus) / HISTORY_MAX;
}

int THistoryHeuristics::Get(int side, const lczero::Move& move) const {
    return FromToHistory[side][move.from().as_int()][move.to().as_int()];
}

void THistoryHeuristics::AddCounterMove(
    const lczero::Move& previousMove,
    const lczero::Move& move
) {
    CounterMoves[previousMove.from().as_int()][previousMove.to().as_int()] = move;
}

lczero::Move THistoryHeuristics::GetCounterMove(const lczero::Move& previousMove) const {
    return CounterMoves[previousMove.from().as_int()][previousMove.to().as_int()];
}

void THistoryHeuristics::Clear() {
    for (auto& side : FromToHistory) {
        for (auto& row : side) {
            row.fill(0);
        }
    }
    for (size_t i = 0; i < 64; i++) {
        CounterMoves[i].fill(lczero::Move());
    }
}
