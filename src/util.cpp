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

bool IsTerminal(
    const lczero::Position& position,
    const std::vector<lczero::Move>& ourLegalMoves
) {
    // HasMatingMaterial considers both sides, one check suffices
    return ourLegalMoves.empty()
        || !position.GetBoard().HasMatingMaterial()
        || position.GetRule50Ply() >= 100;
}


