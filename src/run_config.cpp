#include "warehouse/run_config.hpp"
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace warehouse {

void writeJsonString(std::ostream& output, const std::string& value) {
    output << '"';
    for (unsigned char character : value) {
        switch (character) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (character < 32) {
                const char* digits = "0123456789abcdef";
                output << "\\u00" << digits[character >> 4] << digits[character & 15];
            } else {
                output << character;
            }
        }
    }
    output << '"';
}

void saveRuntimeConfig(const SimulationConfig& config, double maxSeconds) {
    std::filesystem::create_directories(config.output_dir);
    const auto path = config.output_dir / "runtime-config.json";
    std::ofstream output(path);
    if (!output) throw std::runtime_error("Cannot write runtime-config.json");
    output << std::setprecision(17) << std::boolalpha;
    output << "{\n  \"schema_version\": 1,\n  \"mode\": ";
    writeJsonString(output, config.patrol_mode ? "patrol" : "schedule");
    output << ",\n  \"cars\": " << config.unit_count
           << ",\n  \"time_step_seconds\": " << config.time_step
           << ",\n  \"speed_factor\": " << config.speed_factor
           << ",\n  \"seed\": " << config.seed
           << ",\n  \"max_seconds\": " << maxSeconds
           << ",\n  \"write_logs\": " << config.write_logs
           << ",\n  \"console_logging\": " << config.console_logging
           << ",\n  \"output_dir\": ";
    writeJsonString(output, config.output_dir.u8string());
    output << ",\n  \"model\": {\n"
           << "    \"linear_segment_mm\": " << LINEAR_SEGMENT_SIZE << ",\n"
           << "    \"arc_radius_mm\": " << ARC_SECTION_RADIUS << ",\n"
           << "    \"circuit_length_mm\": " << CIRCUIT_TOTAL_LENGTH << ",\n"
           << "    \"linear_limit_mm_per_second\": " << LINEAR_VELOCITY_LIMIT << ",\n"
           << "    \"curve_limit_mm_per_second\": " << CURVE_VELOCITY_LIMIT << ",\n"
           << "    \"acceleration_mm_per_second_squared\": " << ACCELERATION_RATE << ",\n"
           << "    \"deceleration_mm_per_second_squared\": " << DECELERATION_RATE << ",\n"
           << "    \"safety_gap_mm\": " << SAFETY_GAP_MINIMUM << ",\n"
           << "    \"unit_length_mm\": " << TRANSPORT_UNIT_LENGTH << ",\n"
           << "    \"transfer_seconds\": " << CARGO_TRANSFER_DURATION << ",\n"
           << "    \"storage_outbound_seconds\": " << AUTOMATED_STORAGE_OUTBOUND_DURATION << ",\n"
           << "    \"storage_inbound_seconds\": " << AUTOMATED_STORAGE_INBOUND_DURATION << ",\n"
           << "    \"manual_outbound_seconds\": " << MANUAL_OUTBOUND_DURATION << ",\n"
           << "    \"manual_inbound_seconds\": " << MANUAL_INBOUND_DURATION << "\n"
           << "  }\n}\n";
    output.flush();
    if (!output) throw std::runtime_error("Failed to save runtime configuration");
}

} // namespace warehouse
