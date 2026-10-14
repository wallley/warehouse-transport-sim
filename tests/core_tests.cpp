#include "warehouse/simulation.hpp"
#include "warehouse/logging.hpp"
#include "warehouse/run_config.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void near(double actual, double expected, const char* message) {
    check(std::fabs(actual - expected) < 1e-8, message);
}

warehouse::SimulationConfig quietConfig() {
    warehouse::SimulationConfig config;
    config.seed = 42;
    config.write_logs = false;
    config.console_logging = false;
    return config;
}

void geometry() {
    using namespace warehouse;
    near(normalizeCircuitPosition(CIRCUIT_TOTAL_LENGTH + 12), 12, "positive circuit wrap");
    near(normalizeCircuitPosition(-12), CIRCUIT_TOTAL_LENGTH - 12, "negative circuit wrap");
    near(calculateForwardDistance(CIRCUIT_TOTAL_LENGTH - 10, 15), 25, "forward distance across origin");
    near(calculateForwardDistance(40, 40), 0, "same point distance");
    check(isOnCurvedSection(RIGHT_ARC_BEGIN + 1), "right arc classification");
    check(isOnCurvedSection(LEFT_ARC_BEGIN + 1), "left arc classification");
    check(!isOnCurvedSection(UPPER_LINEAR_BEGIN + 1), "linear classification");
    near(getSpeedLimitAtPosition(RIGHT_ARC_BEGIN + 1), CURVE_VELOCITY_LIMIT, "curve speed limit");
    near(getSpeedLimitAtPosition(1), LINEAR_VELOCITY_LIMIT, "linear speed limit");
}

void scenarioAndIsolation() {
    using namespace warehouse;
    SimulationContext first(quietConfig()), second(quietConfig());
    initializeSimulation(first, 3);
    initializeSimulation(second, 3);
    check(first.work_stations.size() == 18, "18 course stations");
    check(first.cargo_orders.size() == 108, "108 course orders");
    int incoming = 0, outgoing = 0;
    for (const auto& order : first.cargo_orders) {
        incoming += order.category == CargoCategory::INCOMING;
        outgoing += order.category == CargoCategory::OUTGOING;
        check(first.work_stations.count(order.sourceStationID) == 1, "valid source");
        check(first.work_stations.count(order.destStationID) == 1, "valid destination");
    }
    check(incoming == 54 && outgoing == 54, "54 incoming and 54 outgoing");
    for (const auto& entry : first.order_queues) {
        int previous = -1;
        for (int index : entry.second) {
            check(index > previous, "source queue preserves task sequence");
            check(first.cargo_orders[index].sourceStationID == entry.first, "source queue ownership");
            previous = index;
        }
    }
    for (int index = 0; index < 2000; ++index) stepSimulation(first);
    near(second.elapsed_time, 0, "other instance clock isolation");
    check(second.search_log_times.empty(), "other instance log cache isolation");
    for (int index = 0; index < 2000; ++index) stepSimulation(second);
    near(first.elapsed_time, second.elapsed_time, "reproducible clock");
    check(first.random_state == second.random_state, "reproducible random stream");
    for (std::size_t index = 0; index < first.transport_units.size(); ++index) {
        const auto& a = first.transport_units[index];
        const auto& b = second.transport_units[index];
        near(a.currentPosition, b.currentPosition, "reproducible position");
        near(a.currentVelocity, b.currentVelocity, "reproducible velocity");
        check(a.operatingMode == b.operatingMode, "reproducible mode");
    }
    initializeSimulation(first, 5);
    check(first.transport_units.size() == 5, "reinitialize vehicle count");
    near(first.elapsed_time, 0, "reinitialize clock reset");
    check(first.search_log_times.empty(), "reinitialize cache reset");
    for (const auto& unit : first.transport_units) near(unit.lastStatusChangeTime, 0, "reinitialize statistics origin");
    first.paused = true;
    stepSimulation(first);
    near(first.elapsed_time, 0, "pause preserves time");
}

void patrolAndRandom() {
    using namespace warehouse;
    auto config = quietConfig();
    config.patrol_mode = true;
    SimulationContext context(config);
    initializeSimulation(context, 3);
    check(context.cargo_orders.empty(), "patrol has no cargo tasks");
    for (int index = 0; index < 1000; ++index) stepSimulation(context);
    near(context.elapsed_time, 50, "fixed time step");
    for (const auto& unit : context.transport_units) {
        check(unit.currentPosition >= 0 && unit.currentPosition < CIRCUIT_TOTAL_LENGTH, "position on track");
        check(!unit.isCarryingCargo(true), "patrol carries no cargo");
    }
    SimulationContext rng(quietConfig());
    check(nextRandom(rng) == 175, "MSVC random sequence compatibility");
    std::ostringstream escaped;
    writeJsonString(escaped, "a\"b\\c\n");
    check(escaped.str() == "\"a\\\"b\\\\c\\n\"", "JSON string escaping");
}

void logLifecycle() {
    using namespace warehouse;
    auto config = quietConfig();
    config.write_logs = true;
    config.output_dir = "test-output/lifecycle";
    SimulationContext context(config);
    initializeSimulation(context, 3);
    check(context.system_debug_log.is_open(), "first run debug log open");
    closeSystemLogs(context);
    initializeSimulation(context, 3);
    check(context.system_debug_log.is_open(), "second run debug log reopened");
    closeSystemLogs(context);
    std::ifstream input(config.output_dir / "DeviceStateLog3units.txt");
    int rows = 0;
    std::string line;
    while (std::getline(input, line)) if (line.find("Empty to Has Goods") != std::string::npos) ++rows;
    check(rows == 9, "initial nine source state changes are logged");
}

} // namespace

int main() {
    try {
        geometry();
        scenarioAndIsolation();
        patrolAndRandom();
        logLifecycle();
        std::cout << "Core invariants and lifecycle checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
