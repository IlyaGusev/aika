#include <search/transposition_table.h>

void TTranspositionTable::Insert(
    const lczero::Position& position,
    const TMoveInfo& move,
    size_t depth,
    ENodeType type
) {
    auto it = Data.find(position);
    if (it == Data.end() || depth > it->second.Depth) {
        Data.insert_or_assign(position, TNode(depth, move, type));
    }
}

std::optional<TTranspositionTable::TNode> TTranspositionTable::Find(
    const lczero::Position& position,
    size_t depth,
    ENodeType type
) const {
    auto it = Data.find(position);
    if (it != Data.end() && it->second.Depth >= depth && it->second.Type == type) {
        return it->second;
    }
    return std::nullopt;
}

