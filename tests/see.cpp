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


void CheckSEE(const std::string& fen, const std::string& moveStr, int eval) {
    lczero::ChessBoard board(fen);
    lczero::PositionHistory history;
    history.Reset(board, 0, 0);
    const auto& position = history.Last();

    BOOST_TEST_MESSAGE(fen << " " << moveStr << " " << eval);
    BOOST_REQUIRE_EQUAL(EvaluateSEE(position, lczero::Move(moveStr)), eval);
}


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

    CheckSEE(fen, "e3c5", GetPieceValue(EPieceType::Queen) - GetPieceValue(EPieceType::Bishop));
    CheckSEE(fen, "e3g5", GetPieceValue(EPieceType::Pawn));
    CheckSEE(fen, "h4h7", GetPieceValue(EPieceType::Pawn) - GetPieceValue(EPieceType::Queen));
    BOOST_REQUIRE_EQUAL(
        EvaluateStaticExchange(position, lczero::BoardSquare("h7")),
        0
    );
}

BOOST_AUTO_TEST_CASE( king_cant_take )
{
    CheckSEE(
        "r3r2k/1p1b3p/p3pb2/P1qPn1p1/1n4PQ/1PNBBR2/2P4P/R1N4K w - - 1 9",
        "h4h7",
        GetPieceValue(EPieceType::Pawn)
    );
}

BOOST_AUTO_TEST_CASE( evaluate )
{
    CheckSEE(
        "r1b1r1k1/1p1n1p1p/p2ppb2/P1q2Pp1/1n2P1PQ/1NNBBR2/1PP4P/R6K b - - 0 4",
        "c4e6",
        GetPieceValue(EPieceType::Bishop) - GetPieceValue(EPieceType::Queen)
    );
}

BOOST_AUTO_TEST_CASE( evaluate_external )
{
    CheckSEE(
        "4R3/2r3p1/5bk1/1p1r3p/p2PR1P1/P1BK1P2/1P6/8 b - -",
        "h4g5",
        0
    );
    CheckSEE(
        "4r1k1/5pp1/nbp4p/1p2p2q/1P2P1b1/1BP2N1P/1B2QPPK/3R4 b - -",
        "g5f6",
        GetPieceValue(EPieceType::Knight) - GetPieceValue(EPieceType::Bishop)
    );
    CheckSEE(
        "2r1r1k1/pp1bppbp/3p1np1/q3P3/2P2P2/1P2B3/P1N1B1PP/2RQ1RK1 b - -",
        "d3e4",
        GetPieceValue(EPieceType::Pawn)
    );
    CheckSEE(
        "7r/5qpk/p1Qp1b1p/3r3n/BB3p2/5p2/P1P2P2/4RK1R w - -",
        "e1e8",
        0
    );
    CheckSEE(
        "6rr/6pk/p1Qp1b1p/2n5/1B3p2/5p2/P1P2P2/4RK1R w - -",
        "e1e8",
        -GetPieceValue(EPieceType::Rook)
    );
    CheckSEE(
        "7r/5qpk/2Qp1b1p/1N1r3n/BB3p2/5p2/P1P2P2/4RK1R w - -",
        "e1e8",
        -GetPieceValue(EPieceType::Rook)
    );
}


