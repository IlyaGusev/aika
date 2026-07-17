# /// script
# dependencies = []
# ///
"""MLE Elo estimate from match.py result files.

    uv run estimate.py sf2450.json sf2600.json sf2750.json   # absolute
    uv run estimate.py gate.json                             # head-to-head diff

Files with opponent_elo set yield an absolute rating on that scale; files
without it anchor the opponent at 0, so the result reads as an Elo
difference vs the opponent build. Don't mix the two kinds in one run.
"""
import json
import math
import random
import sys


def loglik(r, games):
    total = 0.0
    for r_opp, s in games:
        e = 1.0 / (1.0 + 10 ** ((r_opp - r) / 400.0))
        e = min(max(e, 1e-9), 1 - 1e-9)
        total += s * math.log(e) + (1 - s) * math.log(1 - e)
    return total


def mle(games, lo, hi):
    # Log-likelihood is unimodal in r: ternary search to 1-Elo precision
    while hi - lo > 1:
        m1 = lo + (hi - lo) / 3
        m2 = hi - (hi - lo) / 3
        if loglik(m1, games) < loglik(m2, games):
            lo = m1
        else:
            hi = m2
    return round((lo + hi) / 2)


def main():
    games = []
    absolute = None
    for path in sys.argv[1:]:
        with open(path) as f:
            data = json.load(f)
        r_opp = data.get("opponent_elo") or data.get("sf_elo")
        if absolute is None:
            absolute = r_opp is not None
        assert absolute == (r_opp is not None), "mixed anchored/head-to-head files"
        for s in data["scores"]:
            games.append((r_opp if r_opp is not None else 0, s))
        n = len(data["scores"])
        label = f"opponent Elo {r_opp}" if r_opp is not None else "opponent"
        print(f"{path}: {sum(data['scores'])}/{n} vs {label}")

    lo, hi = (1000, 3400) if absolute else (-1000, 1000)
    est = mle(games, lo, hi)
    rng = random.Random(42)
    boots = sorted(
        mle([games[rng.randrange(len(games))] for _ in range(len(games))], lo, hi)
        for _ in range(2000)
    )
    ci_lo, ci_hi = boots[50], boots[1949]
    kind = "Estimated Elo" if absolute else "Elo diff vs opponent"
    print(f"\nGames: {len(games)}")
    print(f"{kind}: {est} (95% CI [{ci_lo}, {ci_hi}])")


if __name__ == "__main__":
    main()
