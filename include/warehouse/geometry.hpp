#pragma once

#include "warehouse/config.hpp"

namespace warehouse {

double normalizeCircuitPosition(double position);
double calculateForwardDistance(double from, double to);
bool isOnCurvedSection(double position);
double getSpeedLimitAtPosition(double position);

} // 命名空间 warehouse
