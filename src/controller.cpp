#include "controller.h"

#include <optional>
#include <chess/pgn.h>
#include <chess/position.h>

lczero::PositionHistory ReadPGN(const std::string& pgn) {
    lczero::PgnReader reader;
    lczero::PositionHistory history = reader.ParseLineSimple(pgn);
    return history;
}

std::string FixCastling(
    const lczero::Position& position,
    const std::string& moveString
) {
    bool can_000 = position.GetBoard().castlings().we_can_000();
    bool can_00 = position.GetBoard().castlings().we_can_00();
    if ((moveString == "e8a8" || moveString == "e1a1") && can_000) {
        return "O-O-O";
    }
    if ((moveString == "e8h8" || moveString == "e1h1") && can_00) {
        return "O-O";
    }
    return moveString;
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

    std::optional<lczero::Move> bestMove;
    for (const auto& strategy : Strategies) {
        bestMove = strategy->MakeMove(history);
        if (bestMove) {
            response["score"] = strategy->GetName();
            std::cerr << "Strategy: " << strategy->GetName() << std::endl;
            break;
        }
    }

    // Converting lc0 format into normal move
    if (history.Last().IsBlackToMove()) {
        bestMove->Mirror();
    }

    std::cerr << "Best move: " << bestMove->as_string() << std::endl;
    std::cerr << std::endl;
    std::string moveString = bestMove->as_string();
    moveString = FixCastling(history.Last(), moveString);
    response["best_move"] = moveString;
    callback(drogon::HttpResponse::newHttpJsonResponse(response));
}
