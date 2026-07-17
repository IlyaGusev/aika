#include <search/see.h>

#include <evaluation.h>
#include <util.h>

#include <algorithm>
#include <array>
#include <cstdint>

// Bitboard exchange evaluation: instead of applying real moves (full board
// copies per exchange step), attackers are recomputed from occupancy with
// magic lookups, so x-rays are revealed as capturers leave their squares.

namespace {

constexpr uint64_t Bit(int row, int col) {
    return (row >= 0 && row < 8 && col >= 0 && col < 8)
        ? (uint64_t(1) << (row * 8 + col)) : 0;
}

struct TAttackTables {
    std::array<uint64_t, 64> Knight{};
    std::array<uint64_t, 64> King{};
    // Squares holding a pawn of the given side that attacks the indexed
    // square; "our" pawns attack upwards (increasing rows)
    std::array<uint64_t, 64> OurPawn{};
    std::array<uint64_t, 64> TheirPawn{};
};

constexpr TAttackTables BuildTables() {
    TAttackTables t{};
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const int sq = r * 8 + c;
            t.Knight[sq] =
                Bit(r + 2, c + 1) | Bit(r + 2, c - 1) |
                Bit(r - 2, c + 1) | Bit(r - 2, c - 1) |
                Bit(r + 1, c + 2) | Bit(r + 1, c - 2) |
                Bit(r - 1, c + 2) | Bit(r - 1, c - 2);
            t.King[sq] =
                Bit(r + 1, c - 1) | Bit(r + 1, c) | Bit(r + 1, c + 1) |
                Bit(r, c - 1) | Bit(r, c + 1) |
                Bit(r - 1, c - 1) | Bit(r - 1, c) | Bit(r - 1, c + 1);
            t.OurPawn[sq] = Bit(r - 1, c - 1) | Bit(r - 1, c + 1);
            t.TheirPawn[sq] = Bit(r + 1, c - 1) | Bit(r + 1, c + 1);
        }
    }
    return t;
}

constexpr TAttackTables ATTACKS = BuildTables();

constexpr size_t MIDGAME = static_cast<size_t>(EGamePhase::Midgame);
constexpr std::array<int, 6> PIECE_VALUES = {
    MATERIAL_SCORES[static_cast<size_t>(EPieceType::Pawn)][MIDGAME],
    MATERIAL_SCORES[static_cast<size_t>(EPieceType::Knight)][MIDGAME],
    MATERIAL_SCORES[static_cast<size_t>(EPieceType::Bishop)][MIDGAME],
    MATERIAL_SCORES[static_cast<size_t>(EPieceType::Rook)][MIDGAME],
    MATERIAL_SCORES[static_cast<size_t>(EPieceType::Queen)][MIDGAME],
    MATERIAL_SCORES[static_cast<size_t>(EPieceType::King)][MIDGAME]
};

struct TSeeContext {
    // Per-type boards stay stale as capturers leave; masking with Occ makes
    // that harmless because attackers are always intersected with occupancy
    uint64_t Pieces[6];
    uint64_t ByColor[2]; // 0 = ours, 1 = theirs
    int To;

    explicit TSeeContext(const lczero::ChessBoard& board, int to)
        : To(to)
    {
        Pieces[0] = board.pawns().as_int();
        Pieces[1] = board.knights().as_int();
        Pieces[2] = board.bishops().as_int();
        Pieces[3] = board.rooks().as_int();
        Pieces[4] = board.queens().as_int();
        Pieces[5] = board.kings().as_int();
        ByColor[0] = board.ours().as_int();
        ByColor[1] = board.theirs().as_int();
    }

    uint64_t AttackersTo(uint64_t occ) const {
        uint64_t att = (ATTACKS.OurPawn[To] & Pieces[0] & ByColor[0])
            | (ATTACKS.TheirPawn[To] & Pieces[0] & ByColor[1])
            | (ATTACKS.Knight[To] & Pieces[1])
            | (ATTACKS.King[To] & Pieces[5]);
        const uint64_t diag = Pieces[2] | Pieces[4];
        const uint64_t line = Pieces[3] | Pieces[4];
        if (diag) {
            att |= lczero::ChessBoard::GetBishopAttacksBB(To, occ).as_int() & diag;
        }
        if (line) {
            att |= lczero::ChessBoard::GetRookAttacksBB(To, occ).as_int() & line;
        }
        return att & occ;
    }
};

// Value the given side can win by continuing the exchange on ctx.To, where
// the piece currently standing there is worth standValue; the side may
// decline, so the result is never negative.
int ExchangeValue(const TSeeContext& ctx, uint64_t occ, int standValue, int side) {
    int gain[40];
    int depth = 0;
    while (true) {
        const uint64_t sideAttackers = ctx.AttackersTo(occ) & ctx.ByColor[side];
        if (!sideAttackers || depth == 39) {
            break;
        }
        int type = 0;
        uint64_t chosen = 0;
        for (; type < 6; ++type) {
            chosen = sideAttackers & ctx.Pieces[type];
            if (chosen) {
                break;
            }
        }
        gain[depth++] = standValue;
        standValue = PIECE_VALUES[type];
        occ ^= chosen & -chosen;
        side ^= 1;
    }
    int value = 0;
    while (depth > 0) {
        value = std::max(0, gain[--depth] - value);
    }
    return value;
}

int PieceValueOr(const lczero::ChessBoard& board, lczero::BoardSquare square,
                 EPieceType fallback) {
    const EPieceType piece = GetPieceType(board, square);
    return GetPieceValue(piece != EPieceType::Unknown ? piece : fallback);
}

} // namespace

int EvaluateStaticExchange(
    const lczero::Position& position,
    lczero::BoardSquare toSquare
) {
    const auto& board = position.GetBoard();
    const auto ours = board.ours();
    const auto theirs = board.theirs();
    if (!ours.get(toSquare) && !theirs.get(toSquare)) {
        return 0;
    }
    const TSeeContext ctx(board, toSquare.as_int());
    const uint64_t occ = ours.as_int() | theirs.as_int();
    const int standValue = PieceValueOr(board, toSquare, EPieceType::Pawn);
    return ExchangeValue(ctx, occ, standValue, 0);
}

int EvaluateCaptureSEE(
    const lczero::Position& position,
    const lczero::Move& move
) {
    DENSURE(UNLIKELY(IsCapture(position, move)),
        "Not a capture in capture SEE, move: " << move.as_string()
        << position.DebugString());

    const auto& board = position.GetBoard();
    const int victimValue = PieceValueOr(board, move.to(), EPieceType::Pawn);
    const int attackerValue = GetPieceValue(board, move.from());

    const TSeeContext ctx(board, move.to().as_int());
    const uint64_t occ = (board.ours().as_int() | board.theirs().as_int())
        ^ (uint64_t(1) << move.from().as_int());
    return victimValue - ExchangeValue(ctx, occ, attackerValue, 1);
}

int EvaluateQuietSEE(
    const lczero::Position& position,
    const lczero::Move& move
) {
    DENSURE(UNLIKELY(!IsCapture(position, move)),
        "Capture in quiet SEE, move: " << move.as_string()
        << position.DebugString());

    const auto& board = position.GetBoard();
    const int movedValue = GetPieceValue(board, move.from());

    const TSeeContext ctx(board, move.to().as_int());
    const uint64_t occ = (board.ours().as_int() | board.theirs().as_int())
        ^ (uint64_t(1) << move.from().as_int())
        ^ (uint64_t(1) << move.to().as_int());
    return -ExchangeValue(ctx, occ, movedValue, 1);
}

int EvaluateSEE(
    const lczero::Position& position,
    const lczero::Move& move
) {
    if (IsCapture(position, move)) {
        return EvaluateCaptureSEE(position, move);
    }
    return EvaluateQuietSEE(position, move);
}
