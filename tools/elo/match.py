# /// script
# dependencies = ["chess"]
# ///
"""UCI match runner: engine vs opponent at fixed movetime.

Absolute anchor (vs Stockfish at a known level):
    uv run match.py --opponent ~/engines/stockfish-16.1 --opponent-uci-elo 2600 \
        --games 32 --out sf2600.json

Head-to-head acceptance gate (new vs old build):
    uv run match.py --engine build/aika_uci --opponent /path/to/old/aika_uci \
        --games 100 --out gate.json

Openings alternate colors; results are flushed to --out after every game.
"""
import argparse
import json
import shlex

import chess
import chess.engine

MAX_PLIES = 300

OPENINGS = [
    [],
    ["e2e4", "e7e5"],
    ["e2e4", "c7c5"],
    ["d2d4", "d7d5"],
    ["d2d4", "g8f6"],
    ["e2e4", "e7e6"],
    ["c2c4", "e7e5"],
    ["g1f3", "d7d5"],
    ["e2e4", "c7c6"],
    ["d2d4", "f7f5"],
]


def play_game(engine, opponent, opening, engine_white, movetime):
    board = chess.Board()
    for uci in opening:
        board.push_uci(uci)
    limit = chess.engine.Limit(time=movetime)
    while not board.is_game_over(claim_draw=True) and board.ply() < MAX_PLIES:
        mover = engine if (board.turn == chess.WHITE) == engine_white else opponent
        board.push(mover.play(board, limit).move)
    outcome = board.outcome(claim_draw=True)
    if outcome is None or outcome.winner is None:
        score = 0.5
    else:
        score = 1.0 if (outcome.winner == chess.WHITE) == engine_white else 0.0
    return score, board.result(claim_draw=True), board.ply()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", default="build/aika_uci")
    parser.add_argument("--opponent", required=True)
    parser.add_argument("--opponent-uci-elo", type=int, default=None,
                        help="configure opponent with UCI_LimitStrength at this Elo")
    parser.add_argument("--games", type=int, default=32)
    parser.add_argument("--movetime", type=float, default=0.25)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    def start_engines():
        engine = chess.engine.SimpleEngine.popen_uci(shlex.split(args.engine))
        opponent = chess.engine.SimpleEngine.popen_uci(shlex.split(args.opponent))
        opponent.configure({"Threads": 1})
        if args.opponent_uci_elo is not None:
            opponent.configure({
                "UCI_LimitStrength": True,
                "UCI_Elo": args.opponent_uci_elo,
            })
        return engine, opponent

    engine, opponent = start_engines()
    scores = []
    for i in range(args.games):
        opening = OPENINGS[(i // 2) % len(OPENINGS)]
        engine_white = i % 2 == 0
        try:
            score, res, plies = play_game(
                engine, opponent, opening, engine_white, args.movetime)
        except Exception as e:
            print(f"game {i + 1} ERROR: {e!r}, restarting engines", flush=True)
            for eng in (engine, opponent):
                try:
                    eng.close()
                except Exception:
                    pass
            engine, opponent = start_engines()
            continue
        scores.append(score)
        print(f"game {i + 1}/{args.games} "
              f"engine_{'white' if engine_white else 'black'} "
              f"score={score} result={res} plies={plies} "
              f"total={sum(scores)}/{len(scores)}", flush=True)
        with open(args.out, "w") as f:
            json.dump({
                "engine": args.engine,
                "opponent": args.opponent,
                "opponent_elo": args.opponent_uci_elo,
                "movetime": args.movetime,
                "scores": scores,
            }, f)

    engine.quit()
    opponent.quit()


if __name__ == "__main__":
    main()
