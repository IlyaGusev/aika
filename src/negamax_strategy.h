#pragma once

#include "strategy.h"
#include "util.h"
#include "search/history_heuristics.h"
#include "search/transposition_table.h"

#include <json/value.h>
#include <json/reader.h>

#include <iostream>
#include <fstream>
#include <unordered_map>

class TNegamaxStrategy : public IStrategy {
public:
    struct TNode {
        lczero::Position Position;
        lczero::Move Move;
        size_t Depth = 0;
        size_t TreeNodesCount = 1;

        TNode(const lczero::Position& oldPosition, const lczero::Move& move, size_t depth)
            : Position(oldPosition, move), Move(move), Depth(depth)
        {}

        TNode(const lczero::Position& position, size_t depth)
            : Position(position), Move(), Depth(depth)
        {}
    };

    struct TConfig {
        int Depth = 3;
        bool EnableAlphaBeta = false;
        int QuiescenceSearchDepth = 0;
        bool EnablePST = false;
        bool EnableTT = false;

        TConfig() = default;
        TConfig(const std::string& configFile) {
            Json::Value root;
            Json::Reader reader;
            std::ifstream file(configFile);
            bool parsingSuccessful = reader.parse(file, root, false);
            assert(parsingSuccessful);
            Depth = root.get("depth", Depth).asInt();
            EnableAlphaBeta = root.get("enable_alpha_beta", EnableAlphaBeta).asBool();
            EnablePST = root.get("enable_pst", EnablePST).asBool();
            EnableTT = root.get("enable_tt", EnableTT).asBool();
            QuiescenceSearchDepth = root.get("qsearch_depth", QuiescenceSearchDepth).asInt();
        }
    };

private:
    TTranspositionTable TranspositionTable;
    THistoryHeuristics HistoryHeuristics;
    TConfig Config;

public:
    TNegamaxStrategy(const std::string& configFile) : Config(configFile) {}
    TNegamaxStrategy(const TConfig& config) : Config(config) {}

    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) override;

    virtual const char* GetName() const override {
        return "Negamax";
    }

private:
    TMoveInfo Search(
        TNode& node,
        int alpha,
        int beta
    );

    TMoveInfo QuiescenceSearch(
        TNode& node,
        int alpha,
        int beta
    );

    void InsertTransposition(
        const TNode& node,
        const TMoveInfo& bestMove
    );

    int CalcMoveOrder(
        const lczero::Position& position,
        const lczero::Move& move
    ) const;
};
