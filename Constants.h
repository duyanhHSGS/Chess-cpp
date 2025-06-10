#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

const uint64_t AI_SEARCH_DEPTH = 5;
const uint8_t NUMBER_OF_CORES_USED = 4;

enum class AIType {
    RANDOM_MOVER,
    SIMPLE_EVALUATION,
    MINIMAX,
    ALPHA_BETA,
};

const AIType DEFAULT_AI_TYPE = AIType::ALPHA_BETA;

#endif // CONSTANTS_H
