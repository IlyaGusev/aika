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
- Alpha-beta pruning with negamax framework
- Quiescence search for capture sequences
- Move ordering with multiple heuristics (see `src/search/move_ordering.cpp`)
- Transposition table (`src/search/transposition_table.h`)
- History heuristics (`src/search/history_heuristics.h`)
- Killer moves (`src/search/killer_moves.h`)
- Late Move Reduction (LMR)
- Null move pruning
- Static Exchange Evaluation (SEE) (`src/search/see.h`)

**TSearchConfig** (`src/search/config.h`) controls search features via JSON:
- `depth`: Main search depth
- `qsearch_depth`: Quiescence search depth limit
- `enable_alpha_beta`: Toggle alpha-beta pruning
- `enable_tt`: Toggle transposition table
- `enable_pst`: Toggle piece-square tables in evaluation
- `enable_null_move`: Toggle null move pruning
- `enable_lmr`: Toggle late move reduction
- `enable_killers`: Toggle killer move heuristic
- `enable_hh`: Toggle history heuristics
- `enable_see_skip`: Toggle SEE-based quiet move skipping

### Evaluation

Evaluation (`src/evaluation.cpp`) combines:
- Material counting (`CalcMaterialScore`)
- Piece-square tables (`CalcPSTScore` in `src/pst.cpp`)
- Color perspective is handled by negating scores for black

### HTTP API

**Endpoint:** `POST /make_move`

**Parameters:**
- `pgn`: PGN string representing the game history

**Response:** JSON with `best_move` field containing UCI move notation or `#` for game over

The controller (`src/controller.cpp`) parses PGN using lc0's `PgnReader`, checks for game end, then queries strategies.

### lc0 Integration

Critical lc0 types used throughout:
- `lczero::PositionHistory`: Game history with positions
- `lczero::Position`: Board position with move generation
- `lczero::Move`: Move representation with UCI conversion
- `lczero::ChessBoard`: Low-level board representation
- `lczero::MoveList`: Generated legal moves

Moves are generated via `position.GenerateLegalMoves()` and applied via `Position(oldPosition, move)` constructor.

### Testing

Tests are in `tests/` directory:
- `perfit.cpp`: Perft move generation validation
- `search.cpp`: Search algorithm tests
- `see.cpp`: Static exchange evaluation tests
- `pst.cpp`: Piece-square table tests

Each test file compiles to a standalone executable using Boost.Test.

## Code Style

**Comments:**
- Avoid overly verbose code comments
- Only add comments when explaining complex algorithms or non-obvious logic
- Prefer self-documenting code with clear variable and function names
- Don't comment what the code obviously does

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
