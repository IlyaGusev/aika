#include "evaluation.h"

#include <chess/board.h>

#include <array>
#include <iostream>
#include "pst.h"

int Evaluate(
    const lczero::Position& position,
    bool usePST /* = true */
) {
    const auto& board = position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    if (ourLegalMoves.empty()) {
        if (board.IsUnderCheck()) {
            return MIN_SCORE_VALUE;
        }
        return 0.0;
    }

    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();
    if (theirLegalMoves.empty()) {
        if (theirBoard.IsUnderCheck()) {
            return MAX_SCORE_VALUE;
        }
        return 0.0;
    }
    if (!board.HasMatingMaterial() || position.GetRule50Ply() >= 100) {
        return 0.0;
    }

    if (usePST) {
        return CalcPSTScore(position);
    }
    return CalcMaterialScore(position) + CalcMobilityScore(position);
}

int CalcMaterialScore(
    const lczero::Position& position
) {
    const auto& board = position.GetBoard();
    std::array<lczero::BitBoard, static_cast<size_t>(EPieceType::Count)> bitboards;
    bitboards[static_cast<size_t>(EPieceType::Pawn)] = board.pawns();
    bitboards[static_cast<size_t>(EPieceType::Queen)] = board.queens();
    bitboards[static_cast<size_t>(EPieceType::Bishop)] = board.bishops();
    bitboards[static_cast<size_t>(EPieceType::Rook)] = board.rooks();
    bitboards[static_cast<size_t>(EPieceType::Knight)] = board.knights();
    bitboards[static_cast<size_t>(EPieceType::King)] = board.kings();
    constexpr size_t MIDGAME = static_cast<size_t>(EGamePhase::Midgame);
    const auto ours = board.ours();
    const auto theirs = board.theirs();
    int score = 0;
    for (size_t i = 0; i < static_cast<size_t>(EPieceType::Count); i++) {
        const int pieceValue = MATERIAL_SCORES[i][MIDGAME];
        const auto& pieceBitboard = bitboards[i];
        if (i == static_cast<size_t>(EPieceType::Pawn)) {
            score += pieceValue * ((pieceBitboard & ours).count() - (pieceBitboard & theirs).count());
        } else {
            score += pieceValue * ((pieceBitboard & ours).count_few() - (pieceBitboard & theirs).count_few());
        }
    }
    return score;
}

int CalcMobilityScore(
    const lczero::Position& position
) {
    const auto& board = position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();
    static const int MOBILITY_SCORE = 10;
    return ourLegalMoves.size() * MOBILITY_SCORE - theirLegalMoves.size() * MOBILITY_SCORE;
}

int EvaluateCapture(
    const lczero::Position& position,
    const lczero::Move& move
) {
    constexpr size_t midgame = static_cast<size_t>(EGamePhase::Midgame);
    const auto& board = position.GetBoard();

    EPieceType toPiece = GetPieceType(board, move.to());
    EPieceType fromPiece = GetPieceType(board, move.from());
    if (fromPiece == EPieceType::Pawn && move.from().col() != move.to().col()) {
        toPiece = EPieceType::Pawn;
    }
    assert(fromPiece != EPieceType::Unknown && toPiece != EPieceType::Unknown);
    int toScore = MATERIAL_SCORES[static_cast<size_t>(toPiece)][midgame];
    int fromScore = MATERIAL_SCORES[static_cast<size_t>(fromPiece)][midgame];
    if (fromPiece == EPieceType::King) {
        fromScore = MATERIAL_SCORES[static_cast<size_t>(EPieceType::Queen)][midgame] * 2;
    }

    return toScore - fromScore / 10;
}

int GetPieceValue(
    const lczero::ChessBoard& board,
    const lczero::BoardSquare& square
) {
   constexpr size_t midgame = static_cast<size_t>(EGamePhase::Midgame);
   EPieceType piece = GetPieceType(board, square);
   assert(piece != EPieceType::Unknown);
   return MATERIAL_SCORES[static_cast<size_t>(piece)][midgame];
}
