#include <chess/board.h>
#include <chess/position.h>

#include <search_strategy.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {

TSearchConfig DefaultConfig() {
    TSearchConfig config;
    config.Depth = 30;
    config.QuiescenceSearchDepth = 10000;
    config.EnableAlphaBeta = true;
    config.EnableTT = true;
    config.EnablePST = true;
    config.EnableNullMove = true;
    config.EnableLMR = true;
    config.TimeLimitMs = 800;
    return config;
}

std::string MoveToUci(const lczero::Position& position, lczero::Move move) {
    move = position.GetBoard().GetLegacyMove(move);
    if (position.IsBlackToMove()) {
        move.Mirror();
    }
    return move.as_string();
}

bool ApplyUciMove(lczero::PositionHistory& history, const std::string& uci) {
    const auto& position = history.Last();
    for (const auto& move : position.GetBoard().GenerateLegalMoves()) {
        if (MoveToUci(position, move) == uci) {
            history.Append(move);
            return true;
        }
    }
    return false;
}

lczero::PositionHistory ParsePosition(std::istringstream& iss) {
    lczero::PositionHistory history;
    std::string token;
    iss >> token;
    std::string fen;
    if (token == "startpos") {
        fen = lczero::ChessBoard::kStartposFen;
        iss >> token;  // optional "moves"
    } else if (token == "fen") {
        while (iss >> token && token != "moves") {
            if (!fen.empty()) {
                fen += ' ';
            }
            fen += token;
        }
    }
    lczero::ChessBoard board;
    int rule50 = 0;
    int fullMoves = 1;
    board.SetFromFen(fen, &rule50, &fullMoves);
    int gamePly = std::max(0, (fullMoves - 1) * 2) + (board.flipped() ? 1 : 0);
    history.Reset(board, rule50, gamePly);
    while (iss >> token) {
        if (!ApplyUciMove(history, token)) {
            std::cerr << "Illegal move: " << token << std::endl;
            break;
        }
    }
    return history;
}

int ComputeTimeBudgetMs(std::istringstream& iss, bool blackToMove, int& depth) {
    int movetime = 0, wtime = 0, btime = 0, winc = 0, binc = 0;
    std::string token;
    while (iss >> token) {
        if (token == "depth") iss >> depth;
        else if (token == "movetime") iss >> movetime;
        else if (token == "wtime") iss >> wtime;
        else if (token == "btime") iss >> btime;
        else if (token == "winc") iss >> winc;
        else if (token == "binc") iss >> binc;
    }
    if (movetime > 0) {
        return movetime;
    }
    int myTime = blackToMove ? btime : wtime;
    int myInc = blackToMove ? binc : winc;
    if (myTime > 0) {
        // TimeLimitMs is a soft cap (the running iteration completes),
        // so budget conservatively to avoid flagging
        return std::max(20, std::min(myTime / 30 + myInc / 2, myTime / 3));
    }
    return 0;
}

}  // namespace

int main() {
    lczero::InitializeMagicBitboards();

    auto strategy = std::make_unique<TSearchStrategy>(DefaultConfig());
    lczero::PositionHistory history;
    history.Reset(lczero::ChessBoard::kStartposBoard, 0, 0);

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            std::cout << "id name Aika\n"
                      << "id author Ilya Gusev\n"
                      // Single-threaded; advertised so GUIs can set it
                      << "option name Threads type spin default 1 min 1 max 1\n"
                      << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            strategy = std::make_unique<TSearchStrategy>(DefaultConfig());
        } else if (command == "position") {
            history = ParsePosition(iss);
        } else if (command == "go") {
            if (history.ComputeGameResult() != lczero::GameResult::UNDECIDED) {
                std::cout << "bestmove 0000" << std::endl;
                continue;
            }
            int depth = 30;
            int budgetMs = ComputeTimeBudgetMs(iss, history.IsBlackToMove(), depth);
            strategy->SetDepth(depth);
            strategy->SetTimeLimit(budgetMs);
            auto move = strategy->MakeMove(history);
            if (!move) {
                std::cout << "bestmove 0000" << std::endl;
                continue;
            }
            // A qsearch-only iteration can yield a moveless stand-pat
            // result; never emit an illegal move
            std::string uciMove = MoveToUci(history.Last(), move->Move);
            const auto legalMoves = history.Last().GetBoard().GenerateLegalMoves();
            bool legal = false;
            for (const auto& m : legalMoves) {
                if (MoveToUci(history.Last(), m) == uciMove) {
                    legal = true;
                    break;
                }
            }
            if (!legal && !legalMoves.empty()) {
                uciMove = MoveToUci(history.Last(), legalMoves.front());
            }
            std::cout << "info depth " << move->Depth
                      << " score cp " << move->Score
                      << " nodes " << move->NodesCount
                      << " time " << move->TimeMs << "\n"
                      << "bestmove " << uciMove << std::endl;
        } else if (command == "quit") {
            break;
        }
        // setoption, stop, etc. are ignored: search is synchronous
    }
    return 0;
}
