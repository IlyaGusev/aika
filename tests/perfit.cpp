#define BOOST_TEST_MODULE "perfit"

#include <evaluation.h>

#include <boost/test/unit_test.hpp>

#include <chrono>

struct F {
    F() { lczero::InitializeMagicBitboards(); }
};
BOOST_TEST_GLOBAL_FIXTURE( F );


size_t PerfIt(size_t depth, const lczero::Position& position, bool includeEval = false) {
    const auto& board = position.GetBoard();
    const auto& ourLegalMoves = board.GenerateLegalMoves();
    if (depth == 0) {
        if (includeEval) {
            const auto& theirBoard = position.GetThemBoard();
            const auto& theirLegalMoves = theirBoard.GenerateLegalMoves();

            const int staticScore = Evaluate(
                position, ourLegalMoves,
                theirLegalMoves, true
            );
            UNUSED(staticScore);
        }
        return 1;
    }
    size_t count = 0;
    for (const auto& move: ourLegalMoves) {
        count += PerfIt(depth - 1, lczero::Position(position, move));
    }
    return count;
}

BOOST_AUTO_TEST_CASE( perfit )
{
    lczero::PositionHistory history;
    lczero::ChessBoard startBoard{lczero::ChessBoard::kStartposFen};
    history.Reset(startBoard, 0, 0);

    auto timerStart = std::chrono::high_resolution_clock::now();

    size_t posCount = PerfIt(6, history.Last());
    BOOST_CHECK_EQUAL(posCount, 119060324);

    auto timerEnd = std::chrono::high_resolution_clock::now();
    size_t timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        timerEnd - timerStart
    ).count();

    BOOST_WARN_MESSAGE(false, "Positions: " << posCount << "; time: " << timeMs);
}
