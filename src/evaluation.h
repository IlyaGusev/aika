#pragma once

#include <chess/position.h>

int Evaluate(
    const lczero::Position& position,
    bool isMirrored = false,
    bool usePST = true
);

bool IsTerminal(
    const lczero::Position& position,
    bool isMirrored = false
);

bool IsCapture(const lczero::Position& position, const lczero::Move& move);
