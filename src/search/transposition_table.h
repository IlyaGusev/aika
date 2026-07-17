#pragma once

#include <strategy.h>
#include <util.h>

#include <cstdint>
#include <vector>

class TTranspositionTable {
public:
    enum class ENodeType : std::int8_t {
        Unknown = -1,
        PV = 0,
        Cut,
        All
    };

    static constexpr int32_t UNKNOWN_EVAL = INT32_MIN;

    struct TEntry {
        uint64_t Hash = 0;
        int32_t Score = 0;
        int32_t StaticEval = UNKNOWN_EVAL;
        lczero::Move Move;
        int16_t Depth = 0;
        ENodeType Type = ENodeType::Unknown;
    };

private:
    static constexpr size_t SIZE = size_t(1) << 21;
    std::vector<TEntry> Data;

public:
    TTranspositionTable() : Data(SIZE) {}

    void Prefetch(uint64_t hash) const {
        __builtin_prefetch(&Data[hash & (SIZE - 1)]);
    }

    const TEntry* Find(uint64_t hash) const {
        const TEntry& entry = Data[hash & (SIZE - 1)];
        if (entry.Type != ENodeType::Unknown && entry.Hash == hash) {
            return &entry;
        }
        return nullptr;
    }

    void Insert(
        uint64_t hash,
        const TMoveInfo& move,
        int depth,
        int alpha,
        int beta,
        int32_t staticEval = UNKNOWN_EVAL
    ) {
        ENodeType type = ENodeType::PV;
        if (move.Score <= alpha) {
            type = ENodeType::All;
        } else if (move.Score >= beta) {
            type = ENodeType::Cut;
        }
        TEntry& entry = Data[hash & (SIZE - 1)];
        // Keep the deeper entry for the same position, otherwise always replace
        if (entry.Type == ENodeType::Unknown || entry.Hash != hash || depth > entry.Depth) {
            entry.Hash = hash;
            entry.Score = move.Score;
            entry.StaticEval = staticEval;
            entry.Move = move.Move;
            entry.Depth = static_cast<int16_t>(depth);
            entry.Type = type;
        }
    }

    std::string GetStats() const {
        return "TT: fixed-size table";
    }

    void Clear() {
        std::fill(Data.begin(), Data.end(), TEntry{});
    }
};
