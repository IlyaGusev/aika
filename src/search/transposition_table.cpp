#include "transposition_table.h"

void TTranspositionTable::Insert(
    const lczero::Position& position,
    const TMoveInfo& bestMove,
    size_t depth,
    ENodeType type
) {
    auto it = Data.find(position);
    if (it == Data.end() || it->second.Depth < depth) {
        Data.insert_or_assign(position, TNode(depth, bestMove, type));
    }
}

std::optional<TMoveInfo> TTranspositionTable::Find(
    const lczero::Position& position,
    size_t depth,
    ENodeType type
) const {
    auto it = Data.find(position);
    if (it != Data.end() && it->second.Depth >= depth && it->second.Type == type) {
        return it->second.BestMove;
    }
    return std::nullopt;
}

