#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// 新旧模型共用取样与序列化，避免测试本身引入时序差异。
namespace parity {

inline void jsonString(std::ostream& output, const std::string& value) {
    output << '"';
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (ch < 0x20) {
                const char* digits = "0123456789abcdef";
                output << "\\u00" << digits[ch >> 4] << digits[ch & 15];
            } else {
                output << static_cast<char>(ch);
            }
        }
    }
    output << '"';
}

inline void jsonValue(std::ostream& output, const std::string& value) {
    jsonString(output, value);
}
inline void jsonValue(std::ostream& output, const char* value) {
    jsonString(output, value);
}
inline void jsonValue(std::ostream& output, bool value) {
    output << (value ? "true" : "false");
}
template <typename Value>
void jsonValue(std::ostream& output, const Value& value) {
    if constexpr (std::is_enum_v<Value>) {
        output << static_cast<int>(value);
    } else {
        if constexpr (std::is_floating_point_v<Value>) {
            if (!std::isfinite(value)) {
                throw std::runtime_error("Snapshot contains a non-finite number");
            }
        }
        output << value;
    }
}

class JsonObject {
public:
    explicit JsonObject(std::ostream& output) : output_(output) { output_ << '{'; }
    ~JsonObject() { output_ << '}'; }
    JsonObject(const JsonObject&) = delete;
    JsonObject& operator=(const JsonObject&) = delete;

    void beginField(const std::string& name) {
        if (!first_) output_ << ',';
        first_ = false;
        jsonString(output_, name);
        output_ << ':';
    }
    template <typename Value>
    void field(const std::string& name, const Value& value) {
        beginField(name);
        jsonValue(output_, value);
    }
private:
    std::ostream& output_;
    bool first_ = true;
};

template <typename Order>
void dumpOrder(std::ostream& output, const Order& value) {
    JsonObject object(output);
#define PARITY_FIELD(name) object.field(#name, value.name)
    PARITY_FIELD(orderID);
    PARITY_FIELD(cargoCode);
    PARITY_FIELD(category);
    PARITY_FIELD(sourceStationID);
    PARITY_FIELD(destStationID);
    PARITY_FIELD(progress);
    PARITY_FIELD(sequenceIndex);
    PARITY_FIELD(stationReadyTime);
    PARITY_FIELD(assignedTransportID);
    PARITY_FIELD(collectionCompleteTime);
    PARITY_FIELD(deliveryCompleteTime);
    PARITY_FIELD(cargoRemovedTime);
    PARITY_FIELD(transportHandling);
#undef PARITY_FIELD
}

template <typename Unit>
void dumpUnit(std::ostream& output, const Unit& value) {
    JsonObject object(output);
#define PARITY_FIELD(name) object.field(#name, value.name)
    PARITY_FIELD(unitID);
    PARITY_FIELD(currentPosition);
    PARITY_FIELD(currentVelocity);
    PARITY_FIELD(activeAcceleration);
    PARITY_FIELD(operatingMode);
    PARITY_FIELD(assignedOrderIndex);
    PARITY_FIELD(targetPosition);
    PARITY_FIELD(hasDestination);
    PARITY_FIELD(operationCountdown);
    PARITY_FIELD(patrolRunTime);
    PARITY_FIELD(patrolStopCount);
    PARITY_FIELD(phaseStartTime);
    PARITY_FIELD(phaseStartVelocity);
    PARITY_FIELD(phaseAcceleration);
    PARITY_FIELD(inAccelPhase);
    PARITY_FIELD(patrolTargetSpeed);
    PARITY_FIELD(patrolTargetStation);
    PARITY_FIELD(patrolLapCompleted);
    PARITY_FIELD(patrolStartPosition);
    PARITY_FIELD(patrolDistanceTraveled);
    PARITY_FIELD(idleSearchCooldown);
    PARITY_FIELD(completedTaskCount);
    PARITY_FIELD(totalIdleTime);
    PARITY_FIELD(totalCarryingTime);
    PARITY_FIELD(totalDistanceTraveled);
    PARITY_FIELD(lastStatusChangeTime);
    PARITY_FIELD(lastOperatingMode);
#undef PARITY_FIELD
    object.beginField("patrolMotionHistory");
    output << '[';
    bool first = true;
    for (const auto& motion : value.patrolMotionHistory) {
        if (!first) output << ',';
        first = false;
        JsonObject record(output);
        record.field("startTime", motion.startTime);
        record.field("endTime", motion.endTime);
        record.field("startVelocity", motion.startVelocity);
        record.field("endVelocity", motion.endVelocity);
        record.field("appliedAcceleration", motion.appliedAcceleration);
    }
    output << ']';
    // unitColor、系统日志、粒子和界面缓存不影响业务行为，故不纳入比较。
}

template <typename Station>
void dumpStation(std::ostream& output, const Station& value) {
    JsonObject object(output);
#define PARITY_FIELD(name) object.field(#name, value.name)
    PARITY_FIELD(stationID);
    PARITY_FIELD(stationType);
    PARITY_FIELD(trackPosition);
    PARITY_FIELD(currentMode);
    PARITY_FIELD(activeOrderIndex);
    PARITY_FIELD(operationTimer);
    PARITY_FIELD(idleTimeTotal);
    PARITY_FIELD(occupiedTimeTotal);
    PARITY_FIELD(lastModeChangeTime);
    PARITY_FIELD(stationName);
#undef PARITY_FIELD
}

template <typename Orders, typename Units, typename Stations, typename Queues>
void dumpSnapshot(std::ostream& output, double elapsed, const Orders& orders,
                  const Units& units, const Stations& stations, const Queues& queues) {
    output.imbue(std::locale::classic());
    output << std::defaultfloat << std::setprecision(17);
    JsonObject snapshot(output);
    snapshot.field("elapsed_time", elapsed);
    snapshot.beginField("orders");
    output << '[';
    bool first = true;
    for (const auto& order : orders) {
        if (!first) output << ',';
        first = false;
        dumpOrder(output, order);
    }
    output << ']';
    snapshot.beginField("units");
    output << '[';
    first = true;
    for (const auto& unit : units) {
        if (!first) output << ',';
        first = false;
        dumpUnit(output, unit);
    }
    output << ']';
    snapshot.beginField("stations");
    {
        JsonObject stationMap(output);
        for (const auto& entry : stations) {
            stationMap.beginField(std::to_string(entry.first));
            dumpStation(output, entry.second);
        }
    }
    snapshot.beginField("queues");
    {
        JsonObject queueMap(output);
        for (const auto& entry : queues) {
            queueMap.beginField(std::to_string(entry.first));
            output << '[';
            bool firstIndex = true;
            for (int index : entry.second) {
                if (!firstIndex) output << ',';
                firstIndex = false;
                output << index;
            }
            output << ']';
        }
    }
}

struct ProbeOptions {
    std::string mode = "schedule";
    int units = 3;
    std::uint32_t seed = 42;
    double time_step = 0.05;
    double max_time = 10800.0;
    std::string checkpoints = "0,1,30,60,120,300,500,600,900,1200,1800,3600,7200,10800";
    std::filesystem::path output = "build/parity-snapshot.json";
    bool help = false;
};

inline unsigned long long parseUnsigned(const std::string& text) {
    if (text.empty() || text[0] == '-') throw std::invalid_argument("Expected a nonnegative integer");
    std::size_t used = 0;
    const auto number = std::stoull(text, &used);
    if (used != text.size()) throw std::invalid_argument("Expected an integer");
    return number;
}

inline double parseTime(const std::string& text) {
    std::size_t used = 0;
    const double value = std::stod(text, &used);
    if (used != text.size() || !std::isfinite(value) || value < 0) {
        throw std::invalid_argument("Expected a finite nonnegative time");
    }
    return value;
}

inline ProbeOptions parseOptions(int argc, char** argv) {
    ProbeOptions options;
    bool explicitMaximum = false;
    for (int index = 1; index < argc; ++index) {
        const std::string key = argv[index];
        if (key == "--help") {
            options.help = true;
            continue;
        }
        if (++index >= argc) throw std::invalid_argument("Missing value for " + key);
        const std::string value = argv[index];
        if (key == "--mode") options.mode = value;
        else if (key == "--units") {
            const auto count = parseUnsigned(value);
            if (count == 0 || count > 12) throw std::invalid_argument("Units must be between 1 and 12");
            options.units = static_cast<int>(count);
        } else if (key == "--seed") {
            const auto seed = parseUnsigned(value);
            if (seed > UINT32_MAX) throw std::invalid_argument("Seed exceeds uint32");
            options.seed = static_cast<std::uint32_t>(seed);
        } else if (key == "--max-time") {
            options.max_time = parseTime(value);
            explicitMaximum = true;
        } else if (key == "--checkpoints") options.checkpoints = value;
        else if (key == "--output") options.output = value;
        else throw std::invalid_argument("Unknown argument: " + key);
    }
    if (options.mode != "schedule" && options.mode != "patrol") {
        throw std::invalid_argument("Mode must be schedule or patrol");
    }
    if (options.mode == "patrol") {
        if (options.units != 3) throw std::invalid_argument("Patrol parity requires exactly 3 units");
        if (!explicitMaximum) options.max_time = 1200.0;
    }
    if (options.max_time <= 0 || options.max_time > 10800.0) {
        throw std::invalid_argument("Maximum time must be in (0, 10800]");
    }
    return options;
}

inline void printHelp() {
    std::cout << "--mode schedule|patrol --units 3|5|7 --seed 42 "
                 "--max-time seconds --checkpoints 0,1,30,60,120,300,600 "
                 "--output build/snapshot.json\n";
}

template <typename Units>
bool patrolHalted(const Units& units) {
    if (units.empty()) return false;
    for (const auto& unit : units) {
        using Mode = std::decay_t<decltype(unit.operatingMode)>;
        if (unit.operatingMode != Mode::PATROL_HALT || unit.operationCountdown <= 900000.0) {
            return false;
        }
    }
    return true;
}

template <typename Orders, typename Units, typename Stations, typename Queues, typename Step>
void runProbe(const ProbeOptions& options, const Orders& orders, const Units& units,
              const Stations& stations, const Queues& queues, const double& elapsed,
              Step advance) {
    const auto maxSteps = static_cast<std::uint64_t>(
        std::floor(options.max_time / options.time_step + 1e-9));
    std::set<std::uint64_t> checkpoints;
    std::stringstream points(options.checkpoints);
    std::string point;
    while (std::getline(points, point, ',')) {
        if (point.empty()) throw std::invalid_argument("Empty checkpoint");
        const double time = parseTime(point);
        if (time <= options.max_time) {
            checkpoints.insert(static_cast<std::uint64_t>(std::llround(time / options.time_step)));
        }
    }
    if (!options.output.parent_path().empty()) {
        std::filesystem::create_directories(options.output.parent_path());
    }
    std::ofstream output(options.output, std::ios::binary);
    if (!output) throw std::runtime_error("Cannot create snapshot output");
    output.imbue(std::locale::classic());
    output << std::setprecision(17);
    {
        JsonObject document(output);
        document.field("mode", options.mode);
        document.field("units", options.units);
        document.field("seed", options.seed);
        document.field("time_step", options.time_step);
        document.field("max_time", options.max_time);
        document.beginField("snapshots");
        output << '[';
        bool firstSnapshot = true;
        bool idleRepositionObserved = false;
        bool complete = false;
        std::uint64_t step = 0;
        const auto observe = [&]() {
            for (const auto& unit : units) {
                using Mode = std::decay_t<decltype(unit.operatingMode)>;
                if (unit.assignedOrderIndex == -1 && unit.hasDestination &&
                    unit.operatingMode == Mode::NAVIGATING_TO_SOURCE) {
                    idleRepositionObserved = true;
                }
            }
        };
        const auto record = [&]() {
            if (!firstSnapshot) output << ',';
            firstSnapshot = false;
            JsonObject sample(output);
            sample.field("step", step);
            sample.beginField("state");
            dumpSnapshot(output, elapsed, orders, units, stations, queues);
        };
        observe();
        if (checkpoints.count(step)) record();
        while (step < maxSteps) {
            complete = advance();
            ++step;
            observe();
            if (checkpoints.count(step)) record();
            if (complete || (options.mode == "patrol" && patrolHalted(units))) break;
        }
        output << ']';
        const bool halted = options.mode == "patrol" && patrolHalted(units);
        document.field("termination", complete ? "schedule-complete" :
                       halted ? "patrol-halted" : "time-limit");
        document.field("observed_idle_reposition", idleRepositionObserved);
        std::size_t finished = 0;
        for (const auto& order : orders) {
            using Progress = std::decay_t<decltype(order.progress)>;
            if (order.progress == Progress::FINISHED) ++finished;
        }
        document.field("finished_orders", finished);
        document.beginField("final");
        {
            JsonObject finalSample(output);
            finalSample.field("step", step);
            finalSample.beginField("state");
            dumpSnapshot(output, elapsed, orders, units, stations, queues);
        }
    }
    output << '\n';
    if (!output) throw std::runtime_error("Failed to write snapshot output");
}

} // 命名空间 parity
