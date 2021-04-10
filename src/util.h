#pragma once

#include <chess/position.h>

#include <sstream>
#include <iostream>

#define UNUSED(x) (void)(x)

#ifdef NDEBUG
#define LOG_DEBUG(x)
#else
#define LOG_DEBUG(x) std::cerr << x << std::endl;
#endif

#define LOG_ERROR(x) std::cerr << x << std::endl;

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define ENSURE(CONDITION, MESSAGE)              \
    do {                                        \
        if (!(CONDITION)) {                     \
            std::ostringstream oss;             \
            oss << MESSAGE;                     \
            throw std::runtime_error(oss.str());\
        }                                       \
    } while (false)

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
    const lczero::Position& position,
    const std::vector<lczero::Move>& ourLegalMoves,
    const std::vector<lczero::Move>& theirLegalMoves
);

bool IsCapture(
    const lczero::Position& position,
    const lczero::Move& move
);

bool IsPromotion(
    const lczero::Position& position,
    const lczero::Move& move
);

EPieceType GetPieceType(
    const lczero::ChessBoard& board,
    const lczero::BoardSquare& square
);


