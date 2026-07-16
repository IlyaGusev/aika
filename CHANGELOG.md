# Changelog

## 2026-07-16 — Search optimization pass: time_benchmark 41.7 s → ~0.74 s (~50×)

Depth-8 all-features benchmark (`time_benchmark` / `deterministic_benchmark`,
fixed middlegame FEN, this machine):

|                    | Before (b9ffd15) | After       | Ratio  |
|--------------------|------------------|-------------|--------|
| Nodes              | 11,323,218       | 324,781     | 34.9×  |
| Time               | ~37.5–41.7 s     | ~0.74 s     | ~50×   |
| Server `/make_move` (depth 6) | 150–350 ms | 70–220 ms | ~2×  |

All test suites pass (`pst`, `see`, `perfit`, `search` — including the
hard-asserted `custom_epd` tactical suite at depths 5–7). Bratko-Kopec
unchanged at 8/12.

### Search features added

- **Principal Variation Search (PVS).** First move searched with a full
  window, the rest with zero windows and a re-search on fail-high. Exact:
  the `alpha_beta` on/off equivalence test still passes. The single biggest
  node reduction (see ablations).
- **Check extension.** A move that gives check keeps the parent's depth
  instead of decrementing it (capped at ply < 4×root depth against
  perpetual-check blowup). Costs ~33% nodes on the benchmark but is what
  lets the engine prove the deep mates in `custom_epd` now that pruning is
  aggressive.
- **Mate-distance scoring.** Mate scores are node-relative
  (`MAX_SCORE − plies to mate`), decremented per backup step
  (`AdjustChildScore`), so the engine prefers faster mates. TT-safe because
  distances are relative to the stored node. Neutral on the benchmark;
  a correctness feature.
- **Reverse futility pruning** (depth ≤ 2): return early when static eval is
  ≥ beta + 120·depth. Gated on `enable_null_move`.
- **Futility pruning** (depth 1): skip quiet moves that cannot raise alpha
  (margin 250), with a conservative cannot-give-check pre-filter (enemy-king
  alignment / knight distance / king moves) so the skip happens before the
  child position is even constructed, and checking moves are never pruned.
  Gated on `enable_lmr`. Both futility gates use flags that the `alpha_beta`
  equivalence test runs with disabled, keeping that test exact.
- **Aspiration windows** at the root from depth 5 (±50 cp around the
  previous iteration's score, full-window re-search on fail). Neutral on
  this benchmark — the TT move anchors the root window already — kept for
  the general case.
- **Delta pruning** in quiescence (margin 200).

### Bug fixes

- **Null move pruning never actually worked.** `TryNullMove` built the
  pass-position and then applied a null `Move()` *on top of it* through the
  `TSearchNode` constructor, flipping the side to move back — it searched
  the same side at reduced depth (and corrupted a1/h1 castling rights).
  Its recursive searches did act as accidental threat-detection that two
  `custom_epd` mates depended on; the check extension now covers those
  soundly. Real null move is worth ~1.55× nodes.
- **Quiescence stand-pat was not fail-soft.** When captures existed, qsearch
  returned `max(captures)` even when declining all captures (stand pat) was
  better, systematically inflating attacker scores. Now
  `bestMoveInfo` starts at the static eval.
- **Mate scores had no distance**, so the engine could not distinguish a
  mate in 4 from a mate in 9 (see mate-distance scoring above).

### Performance work (nodes/second)

- **Transposition table rewritten** (`transposition_table.h`, now
  header-only; `transposition_table.cpp` deleted). Was a
  `boost::unordered_map` keyed by full `Position` objects with 2 finds + 1
  insert per node — the top profile entry at >20% of runtime, plus stats
  maps updated on every probe even in Release. Now a fixed-size 2^21-entry
  hash-indexed array (24-byte entries, full 64-bit hash verification,
  depth-preferred replacement for the same position, always-replace
  otherwise), probed once per node. Entries also cache the static eval.
- **Quiescence movegen generates captures only** (PST configs): pseudolegal
  moves are generated and only captures are legality-checked, plus the
  first legal move to detect mate/stalemate — instead of legality-checking
  all ~44 pseudolegal moves per node. `Search` leaf nodes delegate straight
  to qsearch without generating their own move list. ~9% wall time.
- **Packed PST evaluation**: midgame/endgame values (material folded in)
  packed into one int per piece/square, Stockfish-style, halving table
  lookups; iterates piece bitboards instead of scanning 64 squares with
  `GetPieceType`. ~12% wall time.
- SEE runs only for captures of a more valuable attacker in the qsearch
  filter (victim ≥ attacker implies SEE ≥ 0); SEE's per-call
  `ENSURE(IsCapture(...))` invariant checks compiled out in Release (new
  `DENSURE` macro); `IsCapture`/`GetPieceType`/`IsPromotion` inlined into
  `util.h`; move ordering uses `std::sort` (the comparator is already a
  total order) instead of `stable_sort` with its temp buffer; static eval
  computed lazily only where RFP/null-move/futility need it.

### Ablation study

Each feature disabled individually against the full engine, measured on
`time_benchmark` (nodes are deterministic; time is min of 3 runs on a
shared machine, so small deltas are noise):

| Configuration                   | Nodes      | vs full | Time (ms) | vs full |
|---------------------------------|-----------:|--------:|----------:|--------:|
| **Full engine**                 |    324,781 |      1× |       737 |      1× |
| − PVS                           |  2,647,868 |   8.15× |     3,688 |   5.00× |
| − LMR (pre-existing feature)    |  2,438,856 |   7.51× |     3,350 |   4.55× |
| − Futility pruning              |    518,498 |   1.60× |       983 |   1.33× |
| − Null move pruning             |    503,556 |   1.55× |       979 |   1.33× |
| − Reverse futility pruning      |    451,593 |   1.39× |     1,215 |   1.65× |
| − Delta pruning                 |    339,926 |   1.05× |       778 |   1.06× |
| − Qsearch captures-only movegen |    324,781 |      1× |       802 |   1.09× |
| − Packed PST eval               |    324,781 |      1× |       828 |   1.12× |
| − Aspiration windows            |    324,781 |      1× |       719 |   0.98× |
| − TT static-eval cache          |    324,781 |      1× |       721 |   0.98× |
| − Mate-distance scoring         |    324,781 |      1× |       728 |   0.99× |
| − SEE victim≥attacker shortcut  |    324,768 |      1× |       726 |   0.99× |
| − Check extension               |    244,757 |   0.75× |       570 |   0.77× |

Notes:
- PVS and LMR interact: with PVS off, later moves get full windows and LMR
  re-searches explode; either alone is worth ~8× on this position.
- The check extension is the one feature that *costs* nodes (−25% if
  removed); it pays for the `custom_epd` mate positions, which fail
  without it.
- The TT rewrite cannot be toggled (the search is written against the new
  API); at the old baseline the boost-map TT accounted for >20% of runtime
  in gprof, plus 18M `boost::unordered_map` bookkeeping calls.
- Aspiration, TT static-eval cache, mate-distance, and the SEE shortcut are
  neutral on this single benchmark position; kept for general strength,
  per-node cost elsewhere, and correctness respectively.

### Test changes

- `tests/search.cpp` (`custom_epd`): a prediction now passes if it matches
  *any* move in the EPD `bm` list, per the EPD standard, instead of only
  `BestMoves[0]`.
- `tests/data/custom.epd`: Custom 17 ("Mate in four 3") has a verified cook
  — both `Rxg8` and `Rxc7` force mate in 4. The engine found it once it
  could prove mates; the line was walked to actual checkmate
  (1.Rxc7 Rg6 2.Rxg6 Re7 3.Qxh6+ Rh7 4.Qxh7#). `Rxc7` added to the `bm`
  list.
