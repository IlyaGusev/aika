#include "evaluation.h"

#include <chess/board.h>

#include <array>
#include <iostream>
#include "pst.h"

enum EPieceType {
    PT_UNKNOWN = -1,
    PT_PAWN,
    PT_ROOK,
    PT_KNIGHT,
    PT_BISHOP,
    PT_QUEEN,
    PT_KING,
    PT_COUNT
};

constexpr std::array<int, PT_COUNT> MATERIAL_SCORES{{
    100,
    500,
    300,
    300,
    900,
    20000
}};

int Evaluate(
    const lczero::Position& position,
    bool isMirrored /* = false */,
    bool usePST /* = true */
) {
    const auto& board = isMirrored ? position.GetThemBoard() : position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    if (ourLegalMoves.empty()) {
        if (board.IsUnderCheck()) {
            return -MATERIAL_SCORES[PT_KING];
        }
        return 0.0;
    }

    const auto& theirBoard = isMirrored ? position.GetBoard() : position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();
    if (theirLegalMoves.empty()) {
        if (theirBoard.IsUnderCheck()) {
            return MATERIAL_SCORES[PT_KING];
        }
        return 0.0;
    }

    if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
        return 0.0;
    }

    if (usePST) {
        return EvaluatePST(position, isMirrored);
    }

    auto ours = board.ours();
    auto theirs = board.theirs();
    auto pawns = board.pawns();
    auto queens = board.queens();
    auto bishops = board.bishops();
    auto rooks = board.rooks();
    auto knights = board.knights();
    int pawnDiff = MATERIAL_SCORES[PT_PAWN] * ((pawns & ours).count() - (pawns & theirs).count());
    int queenDiff = MATERIAL_SCORES[PT_QUEEN] * ((queens & ours).count() - (queens & theirs).count());
    int bishopDiff = MATERIAL_SCORES[PT_BISHOP] * ((bishops & ours).count() - (bishops & theirs).count());
    int rookDiff = MATERIAL_SCORES[PT_ROOK] * ((rooks & ours).count() - (rooks & theirs).count());
    int knightDiff = MATERIAL_SCORES[PT_KNIGHT] * ((knights & ours).count() - (knights & theirs).count());
    int score = pawnDiff + queenDiff + bishopDiff + rookDiff + knightDiff;

    static const int MOBILITY_SCORE = 10;
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
