#pragma once

#include <chess/position.h>

#include <util.h>

#include <array>

constexpr std::array<
    std::array<int, static_cast<size_t>(EGamePhase::Count)>,
    static_cast<size_t>(EPieceType::Count)
> MATERIAL_SCORES = {{
{ 82, 94 },
{ 337, 281 },
{ 365, 297 },
{ 477, 512 },
{ 1025, 936 },
{ 10000, 10000 }
}};

int CalcPSTScore(const lczero::Position& position);

// Incrementally maintained tapered-eval state: packed mg/eg sums per side
// (side to move first) plus the game phase counter
struct TPstState {
    int Packed[2] = {0, 0};
    int Phase = 0;
};

TPstState CalcPstState(const lczero::Position& position);
// State after the side to move plays `move`, in the child's perspective;
// board is the position BEFORE the move
TPstState ApplyMovePst(
    const TPstState& state,
    const lczero::ChessBoard& board,
    const lczero::Move& move);
int FinalizePstScore(const TPstState& state);
