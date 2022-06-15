#pragma once

#include <chess/bitboard.h>

#include <optional>

using TMovesPair = std::pair<std::optional<lczero::Move>, std::optional<lczero::Move>>;

class TKillerMoves {
private:
    std::vector<std::optional<lczero::Move>> FirstMoves;
    std::vector<std::optional<lczero::Move>> SecondMoves;

public:
    TKillerMoves() : FirstMoves(100, std::nullopt), SecondMoves(100, std::nullopt) {}

    void InsertMove(const lczero::Move& move, size_t ply) {
        SecondMoves[ply] = FirstMoves[ply];
        FirstMoves[ply] = move;
    }

    std::optional<lczero::Move> GetFirstMove(size_t ply) const { return FirstMoves[ply]; }
    std::optional<lczero::Move> GetSecondMove(size_t ply) const { return SecondMoves[ply]; }
    TMovesPair GetMoves(size_t ply) const {
        return {FirstMoves[ply], SecondMoves[ply]};
    }
};
