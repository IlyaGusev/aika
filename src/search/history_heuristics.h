#pragma once

#include <chess/bitboard.h>

#include <util.h>

#include <array>

class THistoryHeuristics {
private:
    // Butterfly from-to table, kept in [-HISTORY_MAX, HISTORY_MAX] by the
    // gravity update so stale scores decay as new evidence arrives
    std::array<std::array<std::array<int, 64>, 64>, 2> FromToHistory;
    std::array<std::array<lczero::Move, 64>, 64> CounterMoves;

public:
    static constexpr int HISTORY_MAX = 512;

    THistoryHeuristics();

    void Add(int side, const lczero::Move& move, int bonus);
    int Get(int side, const lczero::Move& move) const;

    void AddCounterMove(const lczero::Move& previousMove, const lczero::Move& move);
    lczero::Move GetCounterMove(const lczero::Move& previousMove) const;

    void Clear();
};
