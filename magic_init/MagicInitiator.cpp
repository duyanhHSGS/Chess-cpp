// MagicGenerator.cpp
// Extended to generate both rook and bishop magic bitboard tables.

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unordered_set>
#include <random>
#include <bitset>
#include <iomanip>

constexpr int BOARD_SIZE = 64;
constexpr int ROOK_RELEVANT_BITS[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

constexpr int BISHOP_RELEVANT_BITS[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

uint64_t random_uint64() {
    static std::mt19937_64 rng(123456); // Deterministic seed for reproducibility.
    return rng();
}

uint64_t generate_magic_candidate() {
    return random_uint64() & random_uint64() & random_uint64();
}

int popcount(uint64_t bb) {
    return std::bitset<64>(bb).count();
}

std::vector<uint64_t> generate_blocker_combinations(uint64_t mask) {
    std::vector<uint64_t> result;
    int bits = popcount(mask);
    int combos = 1 << bits;
    for (int i = 0; i < combos; ++i) {
        uint64_t blocker = 0;
        int bit_idx = 0;
        for (int sq = 0; sq < 64; ++sq) {
            if ((mask >> sq) & 1) {
                if ((i >> bit_idx) & 1)
                    blocker |= (1ULL << sq);
                ++bit_idx;
            }
        }
        result.push_back(blocker);
    }
    return result;
}

uint64_t rook_mask(int square) {
    int rank = square / 8;
    int file = square % 8;
    uint64_t mask = 0ULL;
    // Exclude border squares.
    for (int r = rank + 1; r <= 6; ++r) mask |= (1ULL << (r * 8 + file));
    for (int r = rank - 1; r >= 1; --r) mask |= (1ULL << (r * 8 + file));
    for (int f = file + 1; f <= 6; ++f) mask |= (1ULL << (rank * 8 + f));
    for (int f = file - 1; f >= 1; --f) mask |= (1ULL << (rank * 8 + f));
    return mask;
}

uint64_t bishop_mask(int square) {
    int rank = square / 8;
    int file = square % 8;
    uint64_t mask = 0ULL;
    // Diagonals inside board edges.
    for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; ++r, ++f) mask |= (1ULL << (r * 8 + f));
    for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; ++r, --f) mask |= (1ULL << (r * 8 + f));
    for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; --r, ++f) mask |= (1ULL << (r * 8 + f));
    for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; --r, --f) mask |= (1ULL << (r * 8 + f));
    return mask;
}

uint64_t rook_attacks(int square, uint64_t blockers) {
    int rank = square / 8;
    int file = square % 8;
    uint64_t attacks = 0ULL;
    for (int r = rank + 1; r < 8; ++r) {
        attacks |= (1ULL << (r * 8 + file));
        if (blockers & (1ULL << (r * 8 + file))) break;
    }
    for (int r = rank - 1; r >= 0; --r) {
        attacks |= (1ULL << (r * 8 + file));
        if (blockers & (1ULL << (r * 8 + file))) break;
    }
    for (int f = file + 1; f < 8; ++f) {
        attacks |= (1ULL << (rank * 8 + f));
        if (blockers & (1ULL << (rank * 8 + f))) break;
    }
    for (int f = file - 1; f >= 0; --f) {
        attacks |= (1ULL << (rank * 8 + f));
        if (blockers & (1ULL << (rank * 8 + f))) break;
    }
    return attacks;
}

uint64_t bishop_attacks(int square, uint64_t blockers) {
    int rank = square / 8;
    int file = square % 8;
    uint64_t attacks = 0ULL;
    for (int r = rank + 1, f = file + 1; r < 8 && f < 8; ++r, ++f) {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f))) break;
    }
    for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; ++r, --f) {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f))) break;
    }
    for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; --r, ++f) {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f))) break;
    }
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; --r, --f) {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f))) break;
    }
    return attacks;
}

struct MagicEntry {
    uint64_t mask;
    uint64_t magic;
    int shift;
    std::vector<uint64_t> attacks;
};

bool find_magic(int square, int relevant_bits, bool is_rook, MagicEntry& out) {
    uint64_t mask = is_rook ? rook_mask(square) : bishop_mask(square);
    auto blockers = generate_blocker_combinations(mask);
    std::vector<uint64_t> ref_attacks(blockers.size());
    for (size_t i = 0; i < blockers.size(); ++i)
        ref_attacks[i] = is_rook ? rook_attacks(square, blockers[i])
                                 : bishop_attacks(square, blockers[i]);

    for (int attempt = 0; attempt < 100000000; ++attempt) {
        uint64_t magic = generate_magic_candidate();
        std::vector<int> used(1 << relevant_bits, -1);
        bool success = true;
        for (size_t i = 0; i < blockers.size(); ++i) {
            int idx = (blockers[i] * magic) >> (64 - relevant_bits);
            if (used[idx] == -1)
                used[idx] = int(i);
            else if (ref_attacks[used[idx]] != ref_attacks[i]) {
                success = false;
                break;
            }
        }
        if (success) {
            out.mask = mask;
            out.magic = magic;
            out.shift = 64 - relevant_bits;
            out.attacks.resize(1 << relevant_bits);
            for (size_t i = 0; i < blockers.size(); ++i) {
                int idx = (blockers[i] * magic) >> (64 - relevant_bits);
                out.attacks[idx] = ref_attacks[i];
            }
            return true;
        }
    }
    return false;
}

int main() {
    std::ofstream header("MagicTables.h");
    std::ofstream source("MagicTables.cpp");
    if (!header || !source) {
        std::cerr << "Failed to open output files." << std::endl;
        return 1;
    }

    header << "#pragma once\n#include <cstdint>\n\n"
           << "struct MagicEntry {\n"
           << "    uint64_t mask;\n"
           << "    uint64_t magic;\n"
           << "    int shift;\n"
           << "    const uint64_t* attacks;\n"
           << "};\n\n"
           << "extern MagicEntry rook_magics[64];\n"
           << "extern MagicEntry bishop_magics[64];\n"
           << "extern uint64_t rook_attack_table[];\n"
           << "extern uint64_t bishop_attack_table[];\n";

    std::vector<MagicEntry> rook_magics(64), bishop_magics(64);
    std::vector<uint64_t> rook_attacks_flat, bishop_attacks_flat;
    std::vector<size_t> rook_offsets(64), bishop_offsets(64);

    for (int sq = 0; sq < 64; ++sq) {
        MagicEntry m;
        if (!find_magic(sq, ROOK_RELEVANT_BITS[sq], true, m)) {
            std::cerr << "Failed to find rook magic for square " << sq << std::endl;
            return 1;
        }
        rook_offsets[sq] = rook_attacks_flat.size();
        rook_attacks_flat.insert(rook_attacks_flat.end(), m.attacks.begin(), m.attacks.end());
        rook_magics[sq] = m;
    }

    for (int sq = 0; sq < 64; ++sq) {
        MagicEntry m;
        if (!find_magic(sq, BISHOP_RELEVANT_BITS[sq], false, m)) {
            std::cerr << "Failed to find bishop magic for square " << sq << std::endl;
            return 1;
        }
        bishop_offsets[sq] = bishop_attacks_flat.size();
        bishop_attacks_flat.insert(bishop_attacks_flat.end(), m.attacks.begin(), m.attacks.end());
        bishop_magics[sq] = m;
    }

    source << "#include \"MagicTables.h\"\n\n";

    // Print rook attack table.
    source << "uint64_t rook_attack_table[] = {\n";
    for (uint64_t atk : rook_attacks_flat)
        source << "  0x" << std::hex << atk << "ULL,\n";
    source << "};\n\n";

    // Print rook magic entries ensuring the offset is printed as a proper hex literal.
    source << "MagicEntry rook_magics[64] = {\n";
    for (int sq = 0; sq < 64; ++sq) {
        source << "  { 0x" << std::hex << rook_magics[sq].mask << "ULL, 0x"
               << rook_magics[sq].magic << "ULL, " << std::dec << rook_magics[sq].shift
               << ", &rook_attack_table[0x" << std::hex << rook_offsets[sq] << "] },\n";
    }
    source << "};\n\n";

    // Print bishop attack table.
    source << "uint64_t bishop_attack_table[] = {\n";
    for (uint64_t atk : bishop_attacks_flat)
        source << "  0x" << std::hex << atk << "ULL,\n";
    source << "};\n\n";

    // Print bishop magic entries with proper hex offset.
    source << "MagicEntry bishop_magics[64] = {\n";
    for (int sq = 0; sq < 64; ++sq) {
        source << "  { 0x" << std::hex << bishop_magics[sq].mask << "ULL, 0x"
               << bishop_magics[sq].magic << "ULL, " << std::dec << bishop_magics[sq].shift
               << ", &bishop_attack_table[0x" << std::hex << bishop_offsets[sq] << "] },\n";
    }
    source << "};\n";

    std::cout << "Rook and bishop magic tables generated successfully!\n";
    return 0;
}
