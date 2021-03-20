#define BOOST_TEST_MODULE "pst"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#include <boost/test/unit_test.hpp>

#include <chess/position.h>

#include <util.h>
#include <pst.h>

#include <iostream>

struct F {
    F() { lczero::InitializeMagicBitboards(); }
};
BOOST_TEST_GLOBAL_FIXTURE( F );

BOOST_AUTO_TEST_CASE( position_opening )
{
    std::vector<std::pair<std::string, int>> cases{{
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0},
        // -15 -> 17 for white pawn
        {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", -32},
        // -23 -> 12 for black pawn
        {"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", -3},
        // -82 material, +23 from black pawn, -36 white pawn
        {"rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2", -95},
        // -8 from pawns, +20 from black queen
        {"rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3", 12}
    }};
    for (const auto& [fen, value] : cases) {
        lczero::ChessBoard board(fen);
        lczero::PositionHistory history;
        history.Reset(board, 0, 0);
        const auto& position = history.Last();
        int pred = CalcPSTScore(position);
        BOOST_REQUIRE_MESSAGE(pred == value, "" << fen << "; true: " << value << "; pred: " << pred);
    }
}

BOOST_AUTO_TEST_CASE( position_endgame)
{
    std::vector<std::pair<std::string, int>> cases{{
        {"4k3/8/4K3/4P3/8/8/8/8 w - - 0 1", -2 + 20 - (-28) + 94},
        {"1k6/7R/2K5/8/8/8/8/8 w - - 0 1", ((((512 + 3) + 23 + 34) * 22) + ((477 + 44) + 2 - 36) * 2) / 24 }
    }};
    for (const auto& [fen, value] : cases) {
        lczero::ChessBoard board(fen);
        lczero::PositionHistory history;
        history.Reset(board, 0, 0);
        const auto& position = history.Last();
        int pred = CalcPSTScore(position);
        BOOST_REQUIRE_MESSAGE(pred == value, "" << fen << "; true: " << value << "; pred: " << pred);
    }
}
