#include <search/transposition_table.h>

void TTranspositionTable::Insert(
    const lczero::Position& position,
    const TMoveInfo& move,
    int depth,
    ENodeType type
) {
    auto it = Data.find(position);
    if (it == Data.end() || depth > it->second.Depth) {
        Data.insert_or_assign(position, TNode(depth, move, type));
    }
}

void TTranspositionTable::Insert(
    const lczero::Position& position,
    const TMoveInfo& move,
    int depth,
    int alpha,
    int beta
) {
    const int score = move.Score;
    ENodeType nodeType = ENodeType::PV;
    if (score <= alpha) {
        nodeType = ENodeType::All;
    } else if (score >= beta) {
        nodeType = ENodeType::Cut;
    }
    Insert(position, move, depth, nodeType);
}

std::optional<TTranspositionTable::TNode> TTranspositionTable::Find(
    const lczero::Position& position,
    int depth
) const {
    auto it = Data.find(position);
    if (it != Data.end() && depth <= it->second.Depth) {
        return it->second;
    }
    return std::nullopt;
}

