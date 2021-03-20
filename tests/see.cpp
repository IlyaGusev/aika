#define BOOST_TEST_MODULE "see"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#include <evaluation.h>
#include <search/see.h>
#include <util.h>

#include <boost/test/unit_test.hpp>

#include <chess/position.h>

#include <iostream>

struct F {
    F() { lczero::InitializeMagicBitboards(); }
};
BOOST_TEST_GLOBAL_FIXTURE( F );

BOOST_AUTO_TEST_CASE( smallest_attacker )
{
    const std::string fen = "r1b1r1k1/1p1n1p1p/p2ppb2/P1q2Pp1/1n2P2Q/1NNBBRP1/1PP4P/R6K w - - 0 4";
    lczero::ChessBoard board(fen);
    BOOST_REQUIRE_EQUAL(board.GetSmallestAttacker(lczero::BoardSquare("d3"))->as_string(), "b4");
    BOOST_REQUIRE_EQUAL(board.GetSmallestAttacker(lczero::BoardSquare("f5"))->as_string(), "e6");
    BOOST_REQUIRE_EQUAL(board.GetSmallestAttacker(lczero::BoardSquare("h4"))->as_string(), "g5");
    BOOST_REQUIRE_EQUAL(board.GetSmallestAttacker(lczero::BoardSquare("c3"))->as_string(), "f6");
}

BOOST_AUTO_TEST_CASE( see_black )
{
    const std::string fen = "r1b1r1k1/1p1n1p1p/p2ppb2/P1q2Pp1/1n2P1PQ/1NNBBR2/1PP4P/R6K b - - 0 4";
    lczero::ChessBoard board(fen);
    lczero::PositionHistory history;
    history.Reset(board, 0, 0);

    const auto& position = history.Last();
    const auto& themBoard = position.GetThemBoard();
    BOOST_REQUIRE_EQUAL(themBoard.GetSmallestAttacker(lczero::BoardSquare("d3"))->as_string(), "b4");
    BOOST_REQUIRE_EQUAL(themBoard.GetSmallestAttacker(lczero::BoardSquare("e3"))->as_string(), "c5");
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("d6")),
        GetPieceValue(EPieceType::Bishop) - GetPieceValue(EPieceType::Knight)
    );
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("c6")),
        GetPieceValue(EPieceType::Knight) - GetPieceValue(EPieceType::Bishop) + GetPieceValue(EPieceType::Pawn)
    );
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("e6")),
        0
    );
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("h5")),
        GetPieceValue(EPieceType::Queen)
    );
}

BOOST_AUTO_TEST_CASE( see_white )
{
    const std::string fen = "r1b1r2k/1p3p1p/p2ppb2/P1q1nPp1/1n2P1PQ/2NBBR2/1PP4P/R1N4K w - - 3 6";
    lczero::ChessBoard board(fen);
    lczero::PositionHistory history;
    history.Reset(board, 0, 0);

    const auto& position = history.Last();
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("c5")),
        GetPieceValue(EPieceType::Queen) - GetPieceValue(EPieceType::Bishop)
    );
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("g5")),
        GetPieceValue(EPieceType::Pawn)
    );
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("h7")),
        0
    );
}

BOOST_AUTO_TEST_CASE( evaluate )
{
    const std::string fen = "r1b1r1k1/1p1n1p1p/p2ppb2/P1q2Pp1/1n2P1PQ/1NNBBR2/1PP4P/R6K b - - 0 4";
    lczero::ChessBoard board(fen);
    lczero::PositionHistory history;
    history.Reset(board, 0, 0);

    const auto& position = history.Last();
    lczero::Move move("c4e6");
    BOOST_REQUIRE_EQUAL(
        EvaluateCaptureSEE(position, lczero::Move("c4e6")),
        GetPieceValue(EPieceType::Bishop) - GetPieceValue(EPieceType::Queen)
    );
}


