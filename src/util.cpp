#include "util.h"

#include <iostream>

bool IsTerminal(
    const lczero::Position& position
) {
    const auto& board = position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

    if (ourLegalMoves.empty()) {
        return true;
    }

    if (theirLegalMoves.empty()) {
        return true;
    }

    if (!board.HasMatingMaterial() || !theirBoard.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
        return true;
    }

    return false;
}

bool IsCapture(
    const lczero::Position& position,
    const lczero::Move& move
) {
    const auto& board = position.GetBoard();
    if (board.theirs().get(move.to())) {
        return true;
    }
    if (board.pawns().get(move.from()) && move.from().col() != move.to().col()) {
        return true;
    }
    return false;
}

EPieceType GetPieceType(
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

