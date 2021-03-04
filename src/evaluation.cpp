#include "evaluation.h"

#include <chess/board.h>

#include <array>
#include <iostream>

enum EPieceType {
    PT_UNKNOWN = 0,
    PT_PAWN = 1,
    PT_ROOK = 2,
    PT_KNIGHT = 3,
    PT_BISHOP = 4,
    PT_QUEEN = 5,
    PT_KING = 6,
    PT_COUNT
};

constexpr std::array<double, PT_COUNT> MATERIAL_SCORES{{
    0.0,
    1.0,
    5.0,
    3.0,
    3.0,
    9.0,
    200.0
}};

double Evaluate(
    const lczero::Position& position,
    bool isMirrored /* = false */
) {
    const auto& board = isMirrored ? position.GetThemBoard() : position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = isMirrored ? position.GetBoard() : position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

    if (ourLegalMoves.empty()) {
        if (board.IsUnderCheck()) {
            return -MATERIAL_SCORES[PT_KING];
        }
        return 0.0;
    }

    if (theirLegalMoves.empty()) {
        if (theirBoard.IsUnderCheck()) {
            return MATERIAL_SCORES[PT_KING];
        }
        return 0.0;
    }

    if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
        return 0.0;
    }

    double score = 0.0;
    for (int i = 7; i >= 0; --i) {
        for (int j = 0; j < 8; ++j) {
            if (!board.ours().get(i, j) && !board.theirs().get(i, j)) {
                continue;
            }
            EPieceType pieceType = PT_UNKNOWN;
            bool isOurs = (board.ours().get(i, j));
            if (board.pawns().get(i, j)) {
                pieceType = PT_PAWN;
            } else if (board.queens().get(i, j)) {
                pieceType = PT_QUEEN;
            } else if (board.bishops().get(i, j)) {
                pieceType = PT_BISHOP;
            } else if (board.rooks().get(i, j)) {
                pieceType = PT_ROOK;
            } else {
                pieceType = PT_KNIGHT;
            }
            if (isOurs) {
                score += MATERIAL_SCORES[pieceType];
            } else {
                score -= MATERIAL_SCORES[pieceType];
            }
        }
    }

    static const double MOBILITY_SCORE = 0.1;
    score += ourLegalMoves.size() * MOBILITY_SCORE - theirLegalMoves.size() * MOBILITY_SCORE;

    return score;
}

bool IsTerminal(
    const lczero::Position& position,
    bool isMirrored /* = false */
) {
    const auto& board = isMirrored ? position.GetThemBoard() : position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = isMirrored ? position.GetBoard() : position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

    if (ourLegalMoves.empty()) {
        return true;
    }

    if (theirLegalMoves.empty()) {
        return true;
    }

    if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
        return true;
    }
    return false;
}
