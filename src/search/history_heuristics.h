#pragma once

#include <chess/bitboard.h>

#include <util.h>

#include <array>

class THistoryHeuristics {
private:
    std::array<std::array<std::array<int, 64>, static_cast<size_t>(EPieceType::Count)>, 2> PieceHistory;
    std::array<std::array<lczero::Move, 64>, 64> CounterMoves;

public:
    THistoryHeuristics();

    void Add(int side, EPieceType piece, const lczero::Move& move, int depth);
    int Get(int side, EPieceType piece, const lczero::Move& move) const;

    void AddCounterMove(const lczero::Move& previousMove, const lczero::Move& move);
    lczero::Move GetCounterMove(const lczero::Move& previousMove) const;

    void Clear();
};

