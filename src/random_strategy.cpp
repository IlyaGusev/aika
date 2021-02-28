#include "random_strategy.h"

#include <algorithm>
#include <random>

std::optional<lczero::Move> TRandomStrategy::MakeMove(
    const lczero::PositionHistory& history
) const {
    const auto& legalMoves = history.Last().GetBoard().GenerateLegalMoves();
    if (legalMoves.empty()) {
        return std::nullopt;
    }
    lczero::MoveList goodMoves;
    std::sample(
        legalMoves.begin(), legalMoves.end(),
        std::back_inserter(goodMoves), 1,
        std::mt19937{std::random_device{}()}
    );
    return goodMoves.at(0);
}
