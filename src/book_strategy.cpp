#include "book_strategy.h"

#include <iostream>

TBookStrategy::TBookStrategy(const std::string& bookPath) {
    if (!bookPath.empty()) {
        std::cerr << "Reading opening database..." << std::endl;
        lczero::PgnReader reader;
        reader.AddPgnFile(bookPath);
        Openings = reader.GetGames();
        std::cerr << Openings.size() << " openings loaded" << std::endl;
    }
}

std::optional<TMoveInfo> TBookStrategy::MakeMove(
    const lczero::PositionHistory& history
) const {
    const auto& currentBoard = history.Last().GetBoard();
    lczero::ChessBoard startBoard{lczero::ChessBoard::kStartposFen};
    for (const auto& opening : Openings) {
        const auto& bookMoves = opening.moves;
        if (history.GetLength() >= bookMoves.size()) {
            continue;
        }
        if (bookMoves.empty()) {
            continue;
        }
        bool hasSamePrefix = true;
        lczero::PositionHistory bookHistory;
        bookHistory.Reset(startBoard, 0, 0);
        auto lastMove = bookMoves[0];
        for (size_t i = 0; i < history.GetLength(); i++) {
            lczero::Move bookMove = bookMoves[i];
            // Converting normal move to lc0 format
            if (bookHistory.IsBlackToMove()) {
                bookMove.Mirror();
            }
            lastMove = bookMove;
            bookHistory.Append(bookMove);
            if (bookHistory.GetPositionAt(i).GetBoard().Hash() != history.GetPositionAt(i).GetBoard().Hash()) {
                hasSamePrefix = false;
                break;
            }
        }
        if (!hasSamePrefix) {
            continue;
        }
        bool isLegal = currentBoard.IsLegalMove(lastMove, currentBoard.GenerateKingAttackInfo());
        if (isLegal) {
            return TMoveInfo(lastMove);
        } else {
            std::cerr << "Illegal book move " << lastMove.as_string() << std::endl;
            std::cerr << currentBoard.DebugString() << std::endl;
        }
    }
    return std::nullopt;
}
