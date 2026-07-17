# Elo measurement protocol

Repeatable strength measurement for Aika, used both to track absolute
strength and to accept or reject engine changes. Requires `build/aika_uci`
(built from this repo) and python-chess (the scripts carry inline uv
metadata, so `uv run tools/elo/match.py ...` just works).

Both engines always get the same fixed `--movetime` (default 250 ms).
Games use a 10-opening book with colors alternated; draws are claimed
automatically (threefold/50-move), games are capped at 300 plies.

## Acceptance gate for engine changes (run this for every search/eval change)

Head-to-head against the current `main` build. Paired same-conditions games
give far tighter error bars than Stockfish matches, so this is the gate:

```bash
git worktree add /tmp/aika-main main
cd /tmp/aika-main && mkdir -p build && cd build && /usr/bin/cmake -DCMAKE_BUILD_TYPE=Release .. && make aika_uci -j8

cd <repo>
uv run tools/elo/match.py --engine build/aika_uci \
    --opponent /tmp/aika-main/build/aika_uci --games 100 --out gate.json
uv run tools/elo/estimate.py gate.json
```

**Accept** a change when the 95% CI lower bound of the Elo diff is above
−30 (non-regression; for changes whose whole point is strength, require
the lower bound above 0). 100 games resolve roughly ±70 Elo; double the
games when the result straddles the threshold.

## Absolute anchor (occasional, e.g. after a batch of accepted changes)

Bracket Aika between Stockfish `UCI_LimitStrength` levels — one clearly
above, one clearly below, one near 50% — 32 games per level:

```bash
for elo in 2450 2600 2750; do
    uv run tools/elo/match.py --opponent ~/engines/stockfish-16.1 \
        --opponent-uci-elo $elo --games 32 --out sf$elo.json
done
uv run tools/elo/estimate.py sf2450.json sf2600.json sf2750.json
```

Shift the bracket if any level scores outside 15–85%, then update the
baseline below and in CLAUDE.md.

## Baseline

2026-07-17, 250 ms/move, Stockfish 16.1, this machine:
18/32 vs 2450, 13.5/32 vs 2600, 8.5/32 vs 2750 →
**2534 Elo, 95% CI [2460, 2601]** on the SF 16.1 UCI_Elo scale.

## Caveats

- The SF `UCI_Elo` scale is Stockfish's internal calibration — not
  directly comparable to CCRL or FIDE ratings. Only compare numbers
  produced by this same protocol (same SF version, same movetime).
- This is a shared machine: wall-clock load affects search depth per
  move. Both engines suffer equally, but avoid running matches next to
  heavy jobs, and never compare runs taken under very different load.
- Stockfish binary: official releases need glibc ≥ 2.33 (this machine has
  2.31), so build from source: `git clone --depth 1 --branch sf_16.1
  https://github.com/official-stockfish/Stockfish && cd Stockfish/src &&
  make -j8 build ARCH=x86-64-avx2`, then copy `stockfish` to
  `~/engines/stockfish-16.1`.
