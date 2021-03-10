#pragma once

#include <chess/position.h>

enum class EPieceType : std::int8_t {
    Unknown = -1,
    Pawn = 0,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    Count
};

enum class EGamePhase : std::uint8_t {
    Midgame,
    Endgame,
    Count
};

bool IsTerminal(
    const lczero::Position& position
);

bool IsCapture(
    const lczero::Position& position,
    const lczero::Move& move
);

EPieceType GetPieceType(
    const lczero::ChessBoard& board,
    const lczero::BoardSquare& square
);
