#define BOOST_TEST_MODULE "search"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <chess/pgn.h>

#include <search_strategy.h>
#include <evaluation.h>
#include <util.h>

#include <iostream>

TMoveInfo GetFenBestMove(const std::string& fen, int depth = 1, bool useAllFeatures = false) {
    lczero::ChessBoard board(fen);
    lczero::PositionHistory history;
    history.Reset(board, 0, 0);

    TSearchConfig config;
    config.Depth = depth;
    if (depth >= 2) {
        config.EnableAlphaBeta = true;
    }
    if (useAllFeatures) {
        config.EnableTT = true;
        config.EnablePST = true;
        config.EnableNullMove = true;
        config.NullMoveDepthReduction = 3;
        config.NullMoveEvalMargin = 0;
        config.EnableLMR = true;
        config.LMRMinMoveNumber = 12;
        config.LMRMinPly = 2;
        config.LMRMinDepth = 2;
        config.EnableHH = false;
        config.QuiescenceSearchDepth = 10000;
    }
    TSearchStrategy strategy(config);

    auto bestMove = strategy.MakeMove(history);
    return *bestMove;
}

struct TEPDRecord {
    std::string ID;
    std::string FEN;
    std::vector<std::string> BM;
    std::vector<std::string> AM;
    std::vector<lczero::Move> BestMoves;
    std::vector<lczero::Move> AvoidMoves;
};

std::vector<TEPDRecord> ParseSimpleEPD(const std::string& fileName) {
    std::vector<TEPDRecord> records;
    std::ifstream file(fileName);
    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == '#') {
            continue;
        }
        std::string fenWithMoves = line.substr(0, line.find(";"));
        std::string other = line.substr(line.find(";") + 2);

        auto bmPos = fenWithMoves.find("bm");
        auto amPos = fenWithMoves.find("am");
        bool isBestMove = bmPos != std::string::npos;
        bool isAvoidMove = amPos != std::string::npos;
        ENSURE((isBestMove && !isAvoidMove) || (isAvoidMove && !isBestMove),
            "EPD not bm or am");
        auto pos = isBestMove ? bmPos : amPos;

        TEPDRecord record;
        record.FEN = fenWithMoves.substr(0, pos - 1);
        std::string moves = fenWithMoves.substr(pos + 3);
        auto& saveField = isBestMove ? record.BM : record.AM;
        boost::split(saveField, moves, boost::is_any_of(" "), boost::token_compress_on);

        if (other.substr(0, 2) == "id") {
            record.ID = other.substr(4, other.size() - 6);
            if (record.ID.find(";") != std::string::npos) {
                record.ID = record.ID.substr(0, record.ID.find(";") - 1);
            }
        }
        try {
            lczero::ChessBoard board(record.FEN);
            for (const std::string& san : record.BM) {
                record.BestMoves.push_back(SanToMove(san, board));
            }
            for (const std::string& san : record.AM) {
                record.AvoidMoves.push_back(SanToMove(san, board));
            }
        } catch(const lczero::Exception& e) {
            continue;
        }
        records.push_back(record);
    }
    return records;
}

struct F {
    F() { lczero::InitializeMagicBitboards(); }
};
BOOST_TEST_GLOBAL_FIXTURE( F );

BOOST_AUTO_TEST_CASE( time_benchmark )
{
    const std::string fen = "r1b1r1k1/1pqn1pbp/p2pp1p1/P7/1n1NPP1Q/2NBBR2/1PP3PP/R6K w - -";
    const std::string move = "f4f5";

    auto bestMove = GetFenBestMove(fen, 7, true);

    BOOST_WARN_MESSAGE(bestMove.TimeMs < 1000, "" << bestMove.TimeMs << " ms, " << bestMove.NodesCount << " nodes");
    BOOST_WARN_EQUAL(bestMove.Move.as_string(), move);
}

BOOST_AUTO_TEST_CASE( deterministic_benchmark )
{
    const std::string fen = "r1b1r1k1/1pqn1pbp/p2pp1p1/P7/1n1NPP1Q/2NBBR2/1PP3PP/R6K w - -";
    const std::string move = "f4f5";

    auto bestMove = GetFenBestMove(fen, 7, true);
    size_t nodesCount = bestMove.NodesCount;
    BOOST_TEST_MESSAGE("Determinisitc benchmark: " << nodesCount << " nodes");
}

BOOST_AUTO_TEST_CASE( alpha_beta )
{
    lczero::PositionHistory history;
    lczero::ChessBoard startBoard{lczero::ChessBoard::kStartposFen};
    history.Reset(startBoard, 0, 0);

    TSearchConfig config1;
    config1.Depth = 3;
    config1.EnableAlphaBeta = false;
    TSearchStrategy strategy1(config1);

    TSearchConfig config2;
    config2.Depth = 3;
    config2.EnableAlphaBeta = true;
    TSearchStrategy strategy2(config2);

    auto bestMove1 = strategy1.MakeMove(history);
    auto bestMove2 = strategy2.MakeMove(history);

    BOOST_CHECK(bestMove1 != std::nullopt);
    BOOST_CHECK(bestMove2 != std::nullopt);
    BOOST_CHECK_EQUAL(bestMove1->Move.as_string(), bestMove2->Move.as_string());
    BOOST_CHECK_EQUAL(bestMove1->Score, bestMove2->Score);
}

BOOST_AUTO_TEST_CASE( custom_epd )
{
    auto records = ParseSimpleEPD(STR(TEST_PATH)"/data/custom.epd");
    BOOST_TEST_MESSAGE("Custom suite: " << records.size() << " records");
    for (const auto& record : records) {
        BOOST_TEST_MESSAGE("Running " << record.ID);
        for (size_t depth = 5; depth <= 7; depth++) {
            auto prediction = GetFenBestMove(record.FEN, depth, true);
            if (!record.BestMoves.empty()) {
                BOOST_CHECK_MESSAGE(
                    prediction.Move == record.BestMoves[0],
                    record.ID << ": " << record.FEN
                        << " true: " << record.BestMoves[0].as_string()
                        << "; pred: " << prediction.Move.as_string()
                        << "; score: " << prediction.Score
                        << "; depth: " << depth
                );
            }
            if (!record.AvoidMoves.empty()) {
                BOOST_CHECK_MESSAGE(
                    prediction.Move.as_string() != record.AvoidMoves[0].as_string(),
                    record.ID << ": " << record.FEN
                        << " avoid: " << record.AvoidMoves[0].as_string()
                        << "; score: " << prediction.Score
                        << "; depth: " << depth
                );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( bratko_kopec )
{
    auto records = ParseSimpleEPD(STR(TEST_PATH)"/data/bratko_kopec.epd");
    size_t passedCount = 0;
    for (const auto& record : records) {
        BOOST_TEST_MESSAGE("Running " << record.ID);
        auto prediction = GetFenBestMove(record.FEN, 6, true).Move;
        if (prediction == record.BestMoves[0]) {
            passedCount += 1;
        }
        BOOST_WARN_MESSAGE(
            prediction == record.BestMoves[0],
            record.ID << ": " << record.FEN
                << " true: " << record.BestMoves[0].as_string()
                << "; pred: " << prediction.as_string()
        );
    }
    BOOST_TEST_MESSAGE("Bratko-Kopec: " << passedCount << "/" << records.size());
}

/*BOOST_AUTO_TEST_CASE( eret )
{
    auto fullTimeMs = 0;
    size_t passedCount = 0;
    auto records = ParseSimpleEPD(STR(TEST_PATH)"/data/eret.epd");
    for (const auto& record : records) {
        BOOST_TEST_MESSAGE("Running " << record.ID);
        auto prediction = GetFenBestMove(record.FEN, 7, true);
        fullTimeMs += prediction.TimeMs;
        if (prediction.Move == record.BestMoves[0]) {
            passedCount += 1;
        }
        BOOST_WARN_MESSAGE(
            prediction.Move == record.BestMoves[0],
            record.ID << ": " << record.FEN
                << " true: " << record.BestMoves[0].as_string()
                << "; pred: " << prediction.Move.as_string()
        );
    }
    BOOST_TEST_MESSAGE("ERET: " << fullTimeMs << " ms, " << passedCount << "/" << records.size());
}*/

/*BOOST_AUTO_TEST_CASE( arasan )
{
    auto fullTimeMs = 0;
    size_t passedCount = 0;
    auto records = ParseSimpleEPD(STR(TEST_PATH)"/data/arasan.epd");
    for (const auto& record : records) {
        BOOST_TEST_MESSAGE("Running " << record.ID);
        auto prediction = GetFenBestMove(record.FEN, 6, true);
        fullTimeMs += prediction.TimeMs;
        if (prediction.Move == record.BestMoves[0]) {
            passedCount += 1;
        }
        BOOST_WARN_MESSAGE(
            prediction.Move == record.BestMoves[0],
            record.ID << ": " << record.FEN
                << " true: " << record.BestMoves[0].as_string()
                << "; pred: " << prediction.Move.as_string()
        );
    }
    BOOST_TEST_MESSAGE("Arasan: " << fullTimeMs << " ms, " << passedCount << "/" << records.size());
}*/

