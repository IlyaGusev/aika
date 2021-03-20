#pragma once

#include <chess/position.h>

#include <pst.h>

const int MIN_SCORE_VALUE = -100000;
const int MAX_SCORE_VALUE = 100000;

int Evaluate(
    const lczero::Position& position,
    bool usePST /* = true */
);

int Evaluate(
    const lczero::Position& position,
    const std::vector<lczero::Move>& ourLegalMoves,
    const std::vector<lczero::Move>& theirLegalMoves,
    bool usePST = true
);

int CalcMaterialScore(
    const lczero::Position& position
);

int CalcMobilityScore(
    const std::vector<lczero::Move>& ourLegalMoves,
    const std::vector<lczero::Move>& theirLegalMoves
);

int EvaluateCapture(
    const lczero::Position& position,
    const lczero::Move& move
);

int GetPieceValue(
    const lczero::ChessBoard& board,
    const lczero::BoardSquare& square
);

inline int GetPieceValue(EPieceType piece) {
   constexpr size_t midgame = static_cast<size_t>(EGamePhase::Midgame);
   return MATERIAL_SCORES[static_cast<size_t>(piece)][midgame];
}
