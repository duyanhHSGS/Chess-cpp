#ifndef MAGIC_TABLES_H
#define MAGIC_TABLES_H

#include <cstdint>

struct MagicEntry {
    uint64_t mask;
    uint64_t magic;
    int shift;
    const uint64_t* attacks;
};

extern MagicEntry rook_magics[64];
extern MagicEntry bishop_magics[64];
extern uint64_t rook_attack_table[];
extern uint64_t bishop_attack_table[];

#endif // MAGIC_TABLES_H
