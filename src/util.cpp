#include <util.h>

bool IsTerminal(
    const lczero::Position& position,
    const std::vector<lczero::Move>& ourLegalMoves,
    const std::vector<lczero::Move>& theirLegalMoves
) {
    if (ourLegalMoves.empty()) {
        return true;
    }

    if (theirLegalMoves.empty()) {
        return true;
    }

    const auto& board = position.GetBoard();
    const auto& theirBoard = position.GetThemBoard();
    const bool weHasMatingMaterial = board.HasMatingMaterial();
    const bool theyHasMatingMaterial = theirBoard.HasMatingMaterial();
    const bool isRule50 = position.GetRule50Ply() >= 100;
    if (!weHasMatingMaterial || !theyHasMatingMaterial || isRule50) {
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
    // En passant
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

