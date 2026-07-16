# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Aika is an amateur-level C++ chess engine with a web GUI. It's built on top of lc0 (Leela Chess Zero) board representation and uses the Drogon web framework for the HTTP server.

**Key technologies:**
- C++17
- lc0 board representation library (in `thirdparty/lc0/src/chess/`)
- Drogon web framework (HTTP controller in `src/controller.cpp`)
- Boost (program_options, filesystem)
- CMake build system
- Web GUI using chess.js and chessboard.js

## Build Commands

**Prerequisites:**
```bash
# Ubuntu/Debian
sudo apt-get install cmake libboost-all-dev build-essential libjsoncpp-dev uuid-dev

# macOS
brew install boost jsoncpp ossp-uuid
```

**Initial setup:**
```bash
git submodule update --init --recursive
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..
```

**Build types:**
- Release build: `cmake -DCMAKE_BUILD_TYPE=Release ..` (uses -Ofast -march=native)
- Debug build: `cmake -DCMAKE_BUILD_TYPE=Debug ..` (includes address sanitizer and undefined behavior sanitizer)

**Running tests:**
```bash
cd build
make test
```

**Running a single test:**
```bash
cd build
./perfit  # or ./search, ./see, ./pst
```

Run test binaries with `--log_level=message` to see benchmark output (times, node counts, EPD suite scores) that ctest hides.

**Environment gotcha (this machine):** `cmake` and `ctest` on PATH are broken pip shims in `~/.local/bin` (`ModuleNotFoundError: No module named 'cmake'`). Use `/usr/bin/cmake` and `/usr/bin/ctest` explicitly.

**Running the server:**
```bash
cd build
./aika --root ../gui --search_config ../search_config.json
# Access at http://127.0.0.1:8000/index.html
```

**Server options:**
- `--port` (default: 8000): HTTP server port
- `--host` (default: 127.0.0.1): HTTP server host
- `--root` (default: ./gui): Path to web GUI files
- `--search_config` (default: search_config.json): Path to search configuration
- `--book` (default: empty): Path to opening book file

## Architecture

### Strategy Pattern for Move Selection

The engine uses a strategy pattern where multiple move selection strategies can be chained:

1. **IStrategy interface** (`src/strategy.h`): Base interface with `MakeMove()` returning `std::optional<TMoveInfo>`
2. **TBookStrategy** (`src/book_strategy.cpp`): Opening book lookup
3. **TSearchStrategy** (`src/search_strategy.cpp`): Main alpha-beta search engine
4. **TRandomStrategy** (`src/random_strategy.cpp`): Fallback random move

Strategies are tried in order in `TController::MakeMove()` until one returns a move. If a strategy returns `std::nullopt`, the next strategy is attempted.

### Search Engine Components

**TSearchStrategy** (`src/search_strategy.h`) is the core engine implementing:
- Alpha-beta pruning with negamax framework, PVS (zero-window searches with
  re-search on fail-high, active under `enable_alpha_beta`)
- Aspiration windows at the root from depth 5 (needs TT + alpha-beta)
- Quiescence search for capture sequences (SEE-filtered, delta pruning,
  fail-soft stand pat); at qsearch nodes with PST eval on, only captures are
  legality-checked instead of generating all legal moves
- Check extension: a move giving check keeps the parent's depth (capped at
  ply 4×depth against perpetual-check blowup)
- Mate-distance scoring: mate scores are node-relative (MAX_SCORE − plies),
  decremented per backup step in `AdjustChildScore`, so faster mates win
- Move ordering with multiple heuristics (see `src/search/move_ordering.cpp`)
- Transposition table: fixed-size (2^21 entries) hash-indexed array with
  depth-preferred same-position replacement; entries cache the static eval
  (`src/search/transposition_table.h`, header-only)
- History heuristics (`src/search/history_heuristics.h`)
- Killer moves (`src/search/killer_moves.h`)
- Late Move Reduction (LMR)
- Null move pruning
- Reverse futility pruning at depth ≤ 2 (gated on `enable_null_move`) and
  futility pruning of quiet non-checking moves at depth 1 (gated on
  `enable_lmr`) — gated on those flags so the `alpha_beta` equivalence test,
  which runs with them off, stays exact
- Static Exchange Evaluation (SEE) (`src/search/see.h`)

**TSearchConfig** (`src/search/config.h`) controls search features via JSON:
- `depth`: Main search depth
- `qsearch_depth`: Quiescence search depth limit
- `enable_alpha_beta`: Toggle alpha-beta pruning
- `enable_tt`: Toggle transposition table
- `enable_pst`: Toggle piece-square tables in evaluation
- `enable_null_move`: Toggle null move pruning (plus `null_move_depth_reduction`, `null_move_eval_margin`)
- `enable_lmr`: Toggle late move reduction (plus `lmr_min_move_number`, `lmr_min_ply`, `lmr_min_depth`, `lmr_reduction`)
- `enable_killers`: Toggle killer move heuristic

Note: `EnableHH` (history heuristics) and `EnableSEESkip`/`SEESkipQuietMargin` exist as struct fields but are NOT parsed from JSON in `src/search/config.cpp` — they can only be set programmatically (e.g. in tests).

### Evaluation

Evaluation (`src/evaluation.cpp`) combines:
- Material counting (`CalcMaterialScore`)
- Piece-square tables (`CalcPSTScore` in `src/pst.cpp`)
- Color perspective is handled by negating scores for black

### HTTP API

**Endpoint:** `POST /make_move`

**Parameters:**
- `pgn`: PGN string representing the game history

**Response:** JSON with `best_move` (UCI notation, or `#` for game over), `score`, `strategy` (name of the strategy that produced the move), `nodes`, `time` (ms), and `depth`

The controller (`src/controller.cpp`) parses PGN using lc0's `PgnReader`, checks for game end, then queries strategies.

### lc0 Integration

Critical lc0 types used throughout:
- `lczero::PositionHistory`: Game history with positions
- `lczero::Position`: Board position with move generation
- `lczero::Move`: Move representation with UCI conversion
- `lczero::ChessBoard`: Low-level board representation
- `lczero::MoveList`: Generated legal moves

Moves are generated via `position.GenerateLegalMoves()` and applied via `Position(oldPosition, move)` constructor.

**Move format gotcha:** lc0 moves are always from the side-to-move's perspective. Before emitting UCI, the controller converts with `board.GetLegacyMove(move)` (castling fix) and `move.Mirror()` when black is to move (see `src/controller.cpp:60-66`).

### Testing

Tests are in `tests/` directory:
- `perfit.cpp`: Perft move generation validation
- `search.cpp`: Search algorithm tests
- `see.cpp`: Static exchange evaluation tests
- `pst.cpp`: Piece-square table tests

Each test file compiles to a standalone executable using Boost.Test. Test data lives in `tests/data/` and is located via the `TEST_PATH` compile definition.

CI (`.github/workflows/cpp.yml`) builds and runs `make test` on every push/PR to `main`.

**What each suite checks:**
- `pst`: `CalcPSTScore` against hand-computed values for opening/endgame FENs (tapered eval)
- `see`: smallest-attacker selection and static exchange evaluation, both colors, incl. external positions
- `perfit`: perft(6) from startpos must equal exactly 119,060,324 (move generation correctness)
- `search`:
  - `alpha_beta`: pruning soundness — identical move+score with alpha-beta on/off at depth 3
  - `custom_epd`: 20 tactical positions (`tests/data/custom.epd`) solved at depths 5–7, hard assertions; per EPD semantics any move listed in `bm` is accepted (Custom 17 has a verified cook: both Rxg8 and Rxc7 mate in 4)
  - `bratko_kopec`: strength suite, 12 of 24 positions enabled in the EPD; misses are soft warnings
  - `time_benchmark` / `deterministic_benchmark`: depth-8 all-features search on a fixed FEN; time and node count are BOOST_WARN only (never fail the suite)

**Baselines (2026-07-16, after the search optimization pass, this machine):** all suites pass with `MAX_DEPTH = 12` (raised from 8 after the pass): time_benchmark / deterministic_benchmark run at depth 12 (~52 s, 24,762,226 nodes — the 1 s warn is aspirational again; depth 8 was ~740 ms / 324,781 nodes, vs 41.7 s / 11.3M nodes at commit b9ffd15), custom_epd at depths 5–11, Bratko-Kopec 9/12 at depth 11. Full search suite ~4 min. perft(6) ~53 s. Server with default `search_config.json` (depth 6) answers `/make_move` in 70–220 ms. This is a shared machine — wall times can inflate ~1.5× under other users' load; node counts are deterministic.

## Code Style

**Comments:**
- Do NOT add obvious comments that restate what the code does
- Only add comments when explaining WHY (design decisions, workarounds) or complex HOW (algorithms)
- Prefer self-documenting code with clear variable and function names
- Examples of obvious comments to avoid:
  - `// set the value` before `value = 5`
  - `// call the function` before `doSomething()`
  - `// loop through items` before `for (item in items)`
  - `// return the result` before `return result`

## Common Development Patterns

**Adding a new search feature:**
1. Add configuration option to `TSearchConfig` in `src/search/config.h`
2. Parse it in `src/search/config.cpp`
3. Implement logic in `TSearchStrategy::Search()` or `QuiescenceSearch()`
4. Add corresponding test in `tests/search.cpp`
5. Update default `search_config.json` if needed

**Board state debugging:**
Use `position.GetBoard().DebugString()` to print ASCII board representation (see examples in `src/controller.cpp:29-32`)

**Move generation:**
```cpp
const auto& legal_moves = position.GenerateLegalMoves();
for (const auto& move : legal_moves) {
    // move.as_string() gives UCI notation
}
```

**Score convention:**
- Positive scores favor the side to move
- Scores are negated when recursing (`-Search(...)`)
- Mate scores use large constants around 100000
