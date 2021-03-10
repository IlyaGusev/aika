#pragma once

#include <chess/position.h>

int EvaluateStaticExchange(
    const lczero::Position& position,
    lczero::BoardSquare toSquare
);

int EvaluateCaptureSEE(
    const lczero::Position& position,
    const lczero::Move& move
);
