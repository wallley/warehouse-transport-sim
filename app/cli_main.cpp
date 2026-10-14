#include "warehouse/simulation.hpp"
#include "warehouse/logging.hpp"
#include "warehouse/run_config.hpp"
#include "command_line.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

double positiveNumber(const std::string& text) {
    std::size_t used = 0;
    const double value = std::stod(text, &used);
    if (used != text.size() || !std::isfinite(value) || value <= 0)
        throw std::invalid_argument("Expected a finite positive number: " + text);
    return value;
}

unsigned long long unsignedNumber(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Expected an unsigned integer: " + text);
    return std::stoull(text);
}

void printHelp() {
    std::cout << "WarehouseTransportCLI - C++17 warehouse course simulation\n"
              << "Usage: WarehouseTransportCLI [options]\n"
              << "  --mode schedule|patrol    Default: schedule\n"
              << "  --cars 3|5|7              Default: 3; patrol requires 3\n"
              << "  --seed <uint32>           Default: 1\n"
              << "  --time-step <seconds>     Default: 0.05\n"
              << "  --max-seconds <seconds>   Default: 10800\n"
              << "  --output <directory>      Default: outputs\n"
              << "  --snapshot <json-path>    Save final state summary\n"
              << "  --no-logs                 Disable task/device/report logs\n"
              << "  --verbose                 Print simulation event log\n"
              << "  --help                    Show this help\n";
}

void saveSnapshot(const warehouse::SimulationContext& context, const std::filesystem::path& path) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if (!output) throw std::runtime_error("Cannot open snapshot output");
    output << std::setprecision(17) << std::boolalpha;
    output << "{\n  \"schema_version\": 1,\n  \"elapsed_seconds\": " << context.elapsed_time
           << ",\n  \"schedule_complete\": " << context.complete << ",\n  \"orders\": [";
    bool first = true;
    for (const auto& order : context.cargo_orders) {
        if (!first) output << ',';
        first = false;
        output << "\n    {\"id\": " << order.orderID << ", \"progress\": " << static_cast<int>(order.progress)
               << ", \"transport_id\": " << order.assignedTransportID
               << ", \"ready_seconds\": " << order.stationReadyTime
               << ", \"collected_seconds\": " << order.collectionCompleteTime
               << ", \"delivered_seconds\": " << order.deliveryCompleteTime
               << ", \"cleared_seconds\": " << order.cargoRemovedTime << '}';
    }
    output << "\n  ],\n  \"units\": [";
    first = true;
    for (const auto& unit : context.transport_units) {
        if (!first) output << ',';
        first = false;
        output << "\n    {\"id\": " << unit.unitID << ", \"mode\": " << static_cast<int>(unit.operatingMode)
               << ", \"position_mm\": " << unit.currentPosition << ", \"velocity_mm_per_second\": " << unit.currentVelocity
               << ", \"completed_tasks\": " << unit.completedTaskCount
               << ", \"distance_mm\": " << unit.totalDistanceTraveled << '}';
    }
    output << "\n  ]\n}\n";
    output.flush();
    if (!output) throw std::runtime_error("Failed to write snapshot");
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto arguments = utf8Arguments(argc, argv);
        warehouse::SimulationConfig config;
        config.console_logging = false;
        double maxSeconds = 10800;
        std::filesystem::path snapshotPath;
        for (std::size_t index = 1; index < arguments.size(); ++index) {
            const std::string option = arguments[index];
            if (option == "--help") { printHelp(); return 0; }
            if (option == "--no-logs") { config.write_logs = false; continue; }
            if (option == "--verbose") { config.console_logging = true; continue; }
            if (index + 1 >= arguments.size()) throw std::invalid_argument("Missing value for " + option);
            const std::string value = arguments[++index];
            if (option == "--mode") {
                if (value != "schedule" && value != "patrol") throw std::invalid_argument("Mode must be schedule or patrol");
                config.patrol_mode = value == "patrol";
            } else if (option == "--cars") {
                const auto count = unsignedNumber(value);
                if (count != 3 && count != 5 && count != 7) throw std::invalid_argument("Cars must be 3, 5 or 7");
                config.unit_count = static_cast<int>(count);
            } else if (option == "--seed") {
                const auto seed = unsignedNumber(value);
                if (seed > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Seed exceeds uint32 range");
                config.seed = static_cast<std::uint32_t>(seed);
            } else if (option == "--time-step") config.time_step = positiveNumber(value);
            else if (option == "--max-seconds") maxSeconds = positiveNumber(value);
            else if (option == "--output") config.output_dir = std::filesystem::u8path(value);
            else if (option == "--snapshot") snapshotPath = std::filesystem::u8path(value);
            else throw std::invalid_argument("Unknown option: " + option);
        }
        if (config.patrol_mode && config.unit_count != 3) throw std::invalid_argument("Patrol mode requires 3 cars");
        if (maxSeconds < config.time_step) throw std::invalid_argument("Maximum time must be at least one time step");
        if (maxSeconds / config.time_step > 100000000)
            throw std::invalid_argument("Run exceeds the limit of 100 million time steps");
        warehouse::saveRuntimeConfig(config, maxSeconds);
        warehouse::SimulationContext context(config);
        warehouse::initializeSimulation(context, config.unit_count);
        while (context.elapsed_time + config.time_step * 0.5 < maxSeconds && !context.complete)
            warehouse::stepSimulation(context);
        if (config.patrol_mode) {
            for (auto& unit : context.transport_units) warehouse::recordPatrolPhase(context, unit);
            warehouse::generatePatrolReport(context);
        } else warehouse::generateScheduleReport(context);
        warehouse::closeSystemLogs(context);
        if (!snapshotPath.empty()) saveSnapshot(context, snapshotPath);
        int finished = 0;
        for (const auto& order : context.cargo_orders)
            if (order.progress == warehouse::OrderProgress::FINISHED) ++finished;
        std::cout << std::fixed << std::setprecision(2)
                  << "mode=" << (config.patrol_mode ? "patrol" : "schedule")
                  << " cars=" << config.unit_count << " seed=" << config.seed
                  << " elapsed_seconds=" << context.elapsed_time
                  << " finished_orders=" << finished << '/' << context.cargo_orders.size()
                  << " status=" << (config.patrol_mode ? "duration_reached" : (context.complete ? "complete" : "timeout")) << '\n';
        return config.patrol_mode || context.complete ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
