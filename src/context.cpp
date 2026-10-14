#include "warehouse/context.hpp"
#include <utility>

namespace warehouse {

SimulationContext::SimulationContext(SimulationConfig initialConfig)
    : config(std::move(initialConfig)), random_state(config.seed) {}

int nextRandom(SimulationContext& context) {
    context.random_state = context.random_state * 214013u + 2531011u;
    return static_cast<int>((context.random_state >> 16u) & 0x7fffu);
}

} // 命名空间 warehouse
