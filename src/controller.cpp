#include "controller.h"
#include "evaluation.h"
#include "pst.h"

#include <optional>
#include <chess/pgn.h>
#include <chess/position.h>

lczero::PositionHistory ReadPGN(const std::string& pgn) {
    lczero::PgnReader reader;
    lczero::PositionHistory history = reader.ParseLineSimple(pgn);
    return history;
}

void TController::MakeMove(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) const {
    Json::Value response;

    std::string pgn = req->getParameter("pgn");
    std::cerr << "PGN: " << pgn << std::endl;

    const auto& history = ReadPGN(pgn);
    std::cerr << "Chess board: " << std::endl;
    std::cerr << history.Last().GetBoard().DebugString();
    std::cerr << "Current score: " << Evaluate(history.Last()) << std::endl;
    std::cerr << "Current PST score: " << EvaluatePST(history.Last()) << std::endl;

    lczero::GameResult result = history.ComputeGameResult();
    if (result == lczero::GameResult::BLACK_WON) {
        std::cerr << "Black won" << std::endl;
    } else if (result == lczero::GameResult::WHITE_WON) {
        std::cerr << "White won" << std::endl;
    } else if (result == lczero::GameResult::DRAW) {
        std::cerr << "Draw" << std::endl;
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
            std::cerr << "Strategy: " << strategy->GetName() << std::endl;
            break;
        }
    }

    // Fix castlings
    bestMove->Move = history.Last().GetBoard().GetLegacyMove(bestMove->Move);
    // Converting lc0 format into normal move
    if (history.Last().IsBlackToMove()) {
        bestMove->Move.Mirror();
    }

    std::string moveString = bestMove->Move.as_string();
    std::cerr << "Best move: " << moveString << std::endl;
    std::cerr << std::endl;

    response["score"] = bestMove->Score;
    response["best_move"] = moveString;
    response["strategy"] = strategyName;
    response["nodes"] = static_cast<int>(bestMove->NodesCount);
    response["time"] = static_cast<int>(bestMove->TimeMs);
    response["depth"] = static_cast<int>(bestMove->Depth);
    callback(drogon::HttpResponse::newHttpJsonResponse(response));
}
