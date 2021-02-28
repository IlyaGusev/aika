#pragma once

#include <chess/position.h>

#include <vector>
#include <memory>
#include <optional>

class IStrategy {
public:
    virtual std::optional<lczero::Move> MakeMove(
        const lczero::PositionHistory& history
    ) const = 0;

    virtual const char* GetName() const = 0;
};

using TStrategies = std::vector<std::unique_ptr<IStrategy>>;
