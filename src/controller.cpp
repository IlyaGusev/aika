#include <controller.h>

#include <chess/pgn.h>
#include <chess/position.h>

#include <evaluation.h>
#include <pst.h>
#include <util.h>

#include <optional>
#include <string>

lczero::PositionHistory ReadPGN(const std::string& pgn) {
    lczero::PgnReader reader;
    lczero::PositionHistory history = reader.ParsePgnText(pgn);
    return history;
}

void TController::MakeMove(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) const {
    Json::Value response;

    std::string pgn = req->getParameter("pgn");
    PRINT_DEBUG("PGN: " << pgn);
    LOG_INFO << "PGN: " << pgn;

    const auto& history = ReadPGN(pgn);
    PRINT_DEBUG("Chess board: ");
    PRINT_DEBUG(history.Last().GetBoard().DebugString());
    PRINT_DEBUG("Current material score: " << CalcMaterialScore(history.Last()));
    PRINT_DEBUG("Current PST score: " << CalcPSTScore(history.Last()));

    lczero::GameResult result = history.ComputeGameResult();
    if (result == lczero::GameResult::BLACK_WON) {
        PRINT_DEBUG("Black won");
    } else if (result == lczero::GameResult::WHITE_WON) {
        PRINT_DEBUG("White won");
    } else if (result == lczero::GameResult::DRAW) {
        PRINT_DEBUG("Draw");
    }
    if (result != lczero::GameResult::UNDECIDED) {
        response["best_move"] = "#";
        callback(drogon::HttpResponse::newHttpJsonResponse(response));
        return;
    }

    std::optional<TMoveInfo> bestMove;
    std::string strategyName;
    for (const auto& strategy : Strategies) {
        bestMove = strategy->MakeMove(history);
        if (bestMove) {
            strategyName = strategy->GetName();
            PRINT_DEBUG("Strategy: " << strategy->GetName());
            break;
        }
    }

    // Fix castlings
    const auto& position = history.Last();
    bestMove->Move = position.GetBoard().GetLegacyMove(bestMove->Move);
    // Converting lc0 format into normal move
    if (position.IsBlackToMove()) {
        bestMove->Move.Mirror();
    }

    std::string moveString = bestMove->Move.as_string();
    PRINT_DEBUG("Best move: " << moveString << std::endl);
    LOG_INFO << "Best move: " << moveString;

    response["score"] = bestMove->Score;
    response["best_move"] = moveString;
    response["strategy"] = strategyName;
    response["nodes"] = static_cast<int>(bestMove->NodesCount);
    response["time"] = static_cast<int>(bestMove->TimeMs);
    response["depth"] = static_cast<int>(bestMove->Depth);
    callback(drogon::HttpResponse::newHttpJsonResponse(response));
}
