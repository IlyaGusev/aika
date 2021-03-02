#pragma once

#include <chess/position.h>

double Evaluate(
    const lczero::Position& position,
    bool isMirrored = false
);
