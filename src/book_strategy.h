#pragma once

#include "strategy.h"

#include <chess/pgn.h>

class TBookStrategy : public IStrategy {
private:
    std::vector<lczero::Opening> Openings;

public:
    TBookStrategy(const std::string& bookPath);

    virtual std::optional<lczero::Move> MakeMove(
        const lczero::PositionHistory& history
    ) const override;

    virtual const char* GetName() const override {
        return "Book";
    }
};
