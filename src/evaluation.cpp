#include <evaluation.h>

#include <chess/board.h>

#include <pst.h>

#include <array>
#include <iostream>

int Evaluate(
    const lczero::Position& position,
    bool usePST /* = true */
) {
    const auto& board = position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    const auto& theirBoard = position.GetThemBoard();
    const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();
    return Evaluate(position, ourLegalMoves, theirLegalMoves, usePST);
}

int Evaluate(
    const lczero::Position& position,
    const std::vector<lczero::Move>& ourLegalMoves,
    const std::vector<lczero::Move>& theirLegalMoves,
    bool usePST /* = true */
) {
    const auto& board = position.GetBoard();
    if (ourLegalMoves.empty()) {
        if (board.IsUnderCheck()) {
            return MIN_SCORE_VALUE;
        }
        return 0.0;
    }

    const auto& theirBoard = position.GetThemBoard();
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
    const int matScore = CalcMaterialScore(position);
    const int mobScore = CalcMobilityScore(ourLegalMoves, theirLegalMoves);
    return matScore + mobScore;
}

int CalcMaterialScore(
    const lczero::Position& position
) {
    const auto& board = position.GetBoard();
    constexpr size_t pieceTypesCount = static_cast<size_t>(EPieceType::Count);
    std::array<lczero::BitBoard, pieceTypesCount> bitboards;
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
        const int pv = MATERIAL_SCORES[i][MIDGAME];
        const auto& pieceBitboard = bitboards[i];
        auto ourPieces = pieceBitboard & ours;
        auto theirPieces = pieceBitboard & theirs;
        if (i == static_cast<size_t>(EPieceType::Pawn)) {
            score += pv * (ourPieces.count() - theirPieces.count());
        } else {
            score += pv * (ourPieces.count_few() - theirPieces.count_few());
        }
    }
    return score;
}

int CalcMobilityScore(
    const std::vector<lczero::Move>& ourLegalMoves,
    const std::vector<lczero::Move>& theirLegalMoves
) {
    constexpr static int MOBILITY_SCORE = 10;
    const auto ourScore = ourLegalMoves.size() * MOBILITY_SCORE;
    const auto theirScore = theirLegalMoves.size() * MOBILITY_SCORE;
    return ourScore - theirScore;
}

int EvaluateCapture(
    const lczero::Position& position,
    const lczero::Move& move
) {
    const auto& board = position.GetBoard();

    const EPieceType fromPiece = GetPieceType(board, move.from());
    const bool fromIsPawn = fromPiece == EPieceType::Pawn;
    const bool fromIsDiagMove = fromIsPawn && move.from().col() != move.to().col();
    EPieceType toPiece = GetPieceType(board, move.to());
    if (fromIsDiagMove && toPiece == EPieceType::Unknown) {
        toPiece = EPieceType::Pawn;
    }
    ENSURE(UNLIKELY(fromPiece != EPieceType::Unknown),
        "Incorrect FROM piece at " << move.from().as_string()
        << board.DebugString());
    ENSURE(UNLIKELY(toPiece != EPieceType::Unknown),
        "Incorrect TO piece at " << move.to().as_string()
        << board.DebugString());
    return GetPieceValue(toPiece) - GetPieceValue(fromPiece);
}

int GetPieceValue(
    const lczero::ChessBoard& board,
    const lczero::BoardSquare& square
) {
   EPieceType piece = GetPieceType(board, square);
   ENSURE(UNLIKELY(piece != EPieceType::Unknown),
        "Incorrect piece at " << square.as_string() << std::endl
        << board.DebugString());
   return GetPieceValue(piece);
}
