#include "see.h"

#include "../src/evaluation.h"

#include <optional>
#include <iostream>

int EvaluateStaticExchange(
    const lczero::Position& position,
    lczero::BoardSquare toSquare
) {
    // For example - position = BLACK, evaluating captures by BLACK
    // But GetSmallestAttacker can check attacks only on BLACK pieces
    // So GetThemBoard on mirrored toSquare

    toSquare.Mirror();
    auto fromSquareOpt = position.GetThemBoard().GetSmallestAttacker(toSquare);
    if (!fromSquareOpt) {
        return 0;
    }
    // Mirror back
    toSquare.Mirror();
    auto fromSquare = *fromSquareOpt;
    fromSquare.Mirror();

    lczero::Move move(fromSquare, toSquare);
    assert(IsCapture(position, move));
    int capturedPieceValue = GetPieceValue(position.GetBoard(), toSquare);

    // newPosition = WHITE
    lczero::Position newPosition(position, move);
    toSquare.Mirror();
    return std::max(0, capturedPieceValue - EvaluateStaticExchange(newPosition, toSquare));
}

int EvaluateCaptureSEE(
    const lczero::Position& position,
    const lczero::Move& move
) {
    const auto& board = position.GetBoard();
    auto toSquare =  move.to();
    int toValue = GetPieceValue(board, toSquare);
    lczero::Position newPosition(position, move);
    toSquare.Mirror();
    return toValue - EvaluateStaticExchange(newPosition, toSquare);
}
