#include <book_strategy.h>
#include <util.h>

#include <string>

TBookStrategy::TBookStrategy(const std::string& bookPath) {
    if (!bookPath.empty()) {
        LOG_DEBUG("Reading opening database...");
        lczero::PgnReader reader;
        reader.AddPgnFile(bookPath);
        Openings = reader.GetGames();
        LOG_DEBUG(Openings.size() << " openings loaded");
    }
}

std::optional<TMoveInfo> TBookStrategy::MakeMove(
    const lczero::PositionHistory& history
) {
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
            auto historyHash = bookHistory.GetPositionAt(i).GetBoard().Hash();
            auto realHash = history.GetPositionAt(i).GetBoard().Hash();
            if (historyHash != realHash) {
                hasSamePrefix = false;
                break;
            }
        }
        if (!hasSamePrefix) {
            continue;
        }
        bool isLegal = currentBoard.IsLegalMove(
            lastMove, currentBoard.GenerateKingAttackInfo()
        );
        if (isLegal) {
            return TMoveInfo(lastMove);
        } else {
            LOG_ERROR("Illegal book move " << lastMove.as_string());
            LOG_ERROR(currentBoard.DebugString());
        }
    }
    return std::nullopt;
}
