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
    InsertCounts[nodeType] += 1;
    Insert(position, move, depth, nodeType);
}

std::optional<TTranspositionTable::TNode> TTranspositionTable::Find(
    const lczero::Position& position,
    int depth
) const {
    auto it = Data.find(position);
    if (it != Data.end() && depth <= it->second.Depth) {
        FindCounts[it->second.Type] += 1;
        return it->second;
    }
    return std::nullopt;
}

std::string TTranspositionTable::GetStats() const {
    std::stringstream stats;
    stats << "TT inserts" << std::endl;
    for (size_t i = 0; i < static_cast<size_t>(ENodeType::Count); i++) {
        ENodeType nodeType = static_cast<ENodeType>(i);
        stats << "\t" << ToString(nodeType) << " " << InsertCounts.at(nodeType) << std::endl;
    }
    stats << "TT finds" << std::endl;
    for (size_t i = 0; i < static_cast<size_t>(ENodeType::Count); i++) {
        ENodeType nodeType = static_cast<ENodeType>(i);
        stats << "\t" << ToString(nodeType) << " " << FindCounts.at(nodeType) << std::endl;
    }
    return stats.str();
}

const char* TTranspositionTable::ToString(ENodeType type) const {
    switch (type) {
        case ENodeType::PV:
            return "PV";
        case ENodeType::Cut:
            return "Cut";
        case ENodeType::All:
            return "All";
        default:
            return "Unknown";
    }
    return "Unknown";
}

