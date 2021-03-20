#pragma once

#include <chess/bitboard.h>

#include <util.h>

#include <array>

class THistoryHeuristics {
private:
    std::array<std::array<std::array<int, 64>, static_cast<size_t>(EPieceType::Count)>, 2> History;

public:
    THistoryHeuristics();

    void Add(int side, EPieceType piece, const lczero::BoardSquare& square, int depth);
    int Get(int side, EPieceType piece, const lczero::BoardSquare& square) const;
    void Clear();
};

