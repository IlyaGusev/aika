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
{ 0, 0 }
}};

int CalcPSTScore(const lczero::Position& position);
