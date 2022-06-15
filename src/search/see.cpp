#include <search/see.h>

#include <evaluation.h>
#include <util.h>

#include <algorithm>
#include <optional>
#include <utility>

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
    ENSURE(UNLIKELY(IsCapture(position, move)),
        "Not a capture in SEE, move: " << move.as_string()
        << position.DebugString());
    const EPieceType toPiece = GetPieceType(position.GetBoard(), toSquare);
    int capturedPieceValue = GetPieceValue(EPieceType::Pawn);
    if (LIKELY(toPiece != EPieceType::Unknown)) {
        capturedPieceValue = GetPieceValue(position.GetBoard(), toSquare);
    }

    // newPosition = WHITE
    lczero::Position newPosition(position, move);
    toSquare.Mirror();
    const int nestedSEEValue = EvaluateStaticExchange(newPosition, toSquare);
    return std::max(0, capturedPieceValue - nestedSEEValue);
}

int EvaluateCaptureSEE(
    const lczero::Position& position,
    const lczero::Move& move
) {
    ENSURE(UNLIKELY(IsCapture(position, move)),
        "Not a capture in capture SEE, move: " << move.as_string()
        << position.DebugString());

    const auto& board = position.GetBoard();
    auto toSquare = move.to();

    const EPieceType toPiece = GetPieceType(board, toSquare);
    int toValue = GetPieceValue(EPieceType::Pawn);
    if (LIKELY(toPiece != EPieceType::Unknown)) {
        toValue = GetPieceValue(board, toSquare);
    }

    lczero::Position newPosition(position, move);
    toSquare.Mirror();
    return toValue - EvaluateStaticExchange(newPosition, toSquare);
}

int EvaluateQuietSEE(
    const lczero::Position& position,
    const lczero::Move& move
) {
    ENSURE(UNLIKELY(!IsCapture(position, move)),
        "Capture in quiet SEE, move: " << move.as_string()
        << position.DebugString());

    auto toSquare = move.to();
    lczero::Position newPosition(position, move);
    toSquare.Mirror();
    return - EvaluateStaticExchange(newPosition, toSquare);
}

int EvaluateSEE(
    const lczero::Position& position,
    const lczero::Move& move
) {
    if (IsCapture(position, move)) {
        return EvaluateCaptureSEE(position, move);
    }
    return EvaluateQuietSEE(position, move);
}
