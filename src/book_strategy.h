#pragma once

#include <strategy.h>

#include <chess/pgn.h>

#include <vector>
#include <string>

class TBookStrategy : public IStrategy {
private:
    std::vector<lczero::Opening> Openings;

public:
    explicit TBookStrategy(const std::string& bookPath);

    std::optional<TMoveInfo> MakeMove(
        const lczero::PositionHistory& history
    ) override;

    const char* GetName() const override {
        return "Book";
    }
};
