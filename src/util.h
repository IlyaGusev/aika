#pragma once

#include <chess/position.h>

#include <sstream>
#include <iostream>

#define UNUSED(x) (void)(x)

#ifdef NDEBUG
#define PRINT_DEBUG(x)
#else
#define PRINT_DEBUG(x) std::cerr << x << std::endl;
#endif

#define PRINT_ERROR(x) std::cerr << x << std::endl;

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

// Debug-only invariant check: the condition is too expensive for hot paths
#ifdef NDEBUG
#define DENSURE(CONDITION, MESSAGE) do {} while (false)
#else
#define DENSURE(CONDITION, MESSAGE) ENSURE(CONDITION, MESSAGE)
#endif

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

// Search-path variant: no opponent move list needed, see Evaluate overload.
bool IsTerminal(
    const lczero::Position& position,
    const std::vector<lczero::Move>& ourLegalMoves
);

inline bool IsCapture(
    const lczero::Position& position,
    const lczero::Move& move
) {
    const auto& board = position.GetBoard();
    if (board.theirs().get(move.to())) {
        return true;
    }
    // En passant
    if (board.pawns().get(move.from()) && move.from().col() != move.to().col()) {
        return true;
    }
    return false;
}

inline bool IsPromotion(
    const lczero::Move& move
) {
    return move.promotion() != lczero::Move::Promotion::None;
}

inline EPieceType GetPieceType(
    const lczero::ChessBoard& board,
    const lczero::BoardSquare& square
) {
    EPieceType pieceType = EPieceType::Unknown;
    if (board.pawns().get(square)) {
        pieceType = EPieceType::Pawn;
    } else if (board.queens().get(square)) {
        pieceType = EPieceType::Queen;
    } else if (board.bishops().get(square)) {
        pieceType = EPieceType::Bishop;
    } else if (board.rooks().get(square)) {
        pieceType = EPieceType::Rook;
    } else if (board.knights().get(square)) {
        pieceType = EPieceType::Knight;
    } else if (board.kings().get(square)) {
        pieceType = EPieceType::King;
    }
    return pieceType;
}


