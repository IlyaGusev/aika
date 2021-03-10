#pragma once

#include <chess/position.h>
#include "pst.h"

const int MIN_SCORE_VALUE = -100000;
const int MAX_SCORE_VALUE = 100000;

int Evaluate(
    const lczero::Position& position,
    bool usePST = true
);

int CalcMaterialScore(
    const lczero::Position& position
);

int CalcMobilityScore(
    const lczero::Position& position
);

bool IsTerminal(
    const lczero::Position& position
);

bool IsCapture(
    const lczero::Position& position,
    const lczero::Move& move
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
   assert(piece != EPieceType::Unknown);
   return MATERIAL_SCORES[static_cast<size_t>(piece)][midgame];
}
