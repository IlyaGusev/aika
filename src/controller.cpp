#include "controller.h"

#include <algorithm>
#include <random>
#include <chess/pgn.h>


void MirrorMove(std::string& move) {
    for (size_t i = 0; i < move.size(); i++) {
        if (move[i] - '0' >= 1 && move[i] - '0' <= 8) {
            size_t num = static_cast<size_t>(move[i] - '0');
            move[i] = static_cast<char>(9 - num) + '0';
        }
    }
}

void TController::MakeMove(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) const {
    std::string pgn = req->getParameter("pgn");
    lczero::PgnReader reader;
    lczero::ChessBoard board{lczero::ChessBoard::kStartposFen};
    lczero::MoveList game;
    std::cerr << "PGN: " << pgn << std::endl;
    reader.ParseLineSimple(pgn, board, game);
    lczero::MoveList legalMoves = board.GenerateLegalMoves();

    lczero::MoveList goodMoves;
    std::sample(
        legalMoves.begin(), legalMoves.end(),
        std::back_inserter(goodMoves), 1,
        std::mt19937{std::random_device{}()}
    );

    std::string bestMove = goodMoves.at(0).as_string();
    // Board for black is mirrored
    if (game.size() % 2 == 1) {
        MirrorMove(bestMove);
    }
    std::cerr << bestMove << std::endl;
    Json::Value ret;
    ret["best_move"] = bestMove;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}


