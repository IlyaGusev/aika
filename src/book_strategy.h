#pragma once

#include "strategy.h"

#include <chess/pgn.h>

class TBookStrategy : public IStrategy {
private:
    std::vector<lczero::Opening> Openings;

public:
    TBookStrategy(const std::string& bookPath);

    virtual std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) override;

    virtual const char* GetName() const override {
        return "Book";
    }
};
