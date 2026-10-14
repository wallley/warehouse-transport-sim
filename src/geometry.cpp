#include "warehouse/simulation.hpp"
#include "warehouse/logging.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace warehouse {

double normalizeCircuitPosition(double pos) {
    pos = fmod(pos, CIRCUIT_TOTAL_LENGTH);
    if (pos < 0) {
        pos += CIRCUIT_TOTAL_LENGTH;
    }
    return pos;
}

double calculateForwardDistance(double from, double to) {
    from = normalizeCircuitPosition(from);
    to = normalizeCircuitPosition(to);
    if (to >= from) {
        return to - from;
    } else {
        return (CIRCUIT_TOTAL_LENGTH - from) + to;
    }
}

bool isOnCurvedSection(double pos) {
    pos = normalizeCircuitPosition(pos);
    return (pos >= RIGHT_ARC_BEGIN && pos < RIGHT_ARC_END) ||
           (pos >= LEFT_ARC_BEGIN && pos < LEFT_ARC_END);
}

double getSpeedLimitAtPosition(double pos) {
    return isOnCurvedSection(pos) ? CURVE_VELOCITY_LIMIT : LINEAR_VELOCITY_LIMIT;
}

} // 命名空间 warehouse
