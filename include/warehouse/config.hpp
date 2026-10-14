#pragma once

#include <cstdint>
#include <filesystem>

namespace warehouse {

using Color = std::uint32_t;

// 颜色采用与 Win32 COLORREF 相同的通道布局，核心无需包含图形头文件。
constexpr Color rgb(unsigned int red, unsigned int green, unsigned int blue) {
    return (red & 255u) | ((green & 255u) << 8u) | ((blue & 255u) << 16u);
}

inline constexpr double MATH_PI = 3.141592653589793;
inline constexpr double LINEAR_SEGMENT_SIZE = 40000.0;
inline constexpr double ARC_SECTION_RADIUS = 2500.0;
inline constexpr double ARC_SECTION_LENGTH = MATH_PI * ARC_SECTION_RADIUS;
inline constexpr int LOG_BUFFER_SIZE = 20;
inline constexpr double UPPER_LINEAR_BEGIN = 0.0;
inline constexpr double UPPER_LINEAR_END = UPPER_LINEAR_BEGIN + LINEAR_SEGMENT_SIZE;
inline constexpr double RIGHT_ARC_BEGIN = UPPER_LINEAR_END;
inline constexpr double RIGHT_ARC_END = RIGHT_ARC_BEGIN + ARC_SECTION_LENGTH;
inline constexpr double LOWER_LINEAR_BEGIN = RIGHT_ARC_END;
inline constexpr double LOWER_LINEAR_END = LOWER_LINEAR_BEGIN + LINEAR_SEGMENT_SIZE;
inline constexpr double LEFT_ARC_BEGIN = LOWER_LINEAR_END;
inline constexpr double LEFT_ARC_END = LEFT_ARC_BEGIN + ARC_SECTION_LENGTH;
inline constexpr double CIRCUIT_TOTAL_LENGTH = LEFT_ARC_END;
inline constexpr double LINEAR_VELOCITY_LIMIT = 160.0 * 1000.0 / 60.0;
inline constexpr double CURVE_VELOCITY_LIMIT = 40.0 * 1000.0 / 60.0;
inline constexpr double ACCELERATION_RATE = 0.5 * 1000.0;
inline constexpr double DECELERATION_RATE = 0.5 * 1000.0;
inline constexpr double SAFETY_GAP_MINIMUM = 2200.0;
inline constexpr double CARGO_TRANSFER_DURATION = 7.5;
inline constexpr double TRANSPORT_UNIT_LENGTH = 2000.0;
inline constexpr double AUTOMATED_STORAGE_OUTBOUND_DURATION = 50.0;
inline constexpr double AUTOMATED_STORAGE_INBOUND_DURATION = 25.0;
inline constexpr double MANUAL_OUTBOUND_DURATION = 30.0;
inline constexpr double MANUAL_INBOUND_DURATION = 30.0;
inline constexpr Color BG_DARK = rgb(5, 0, 15);
inline constexpr Color BG_MID = rgb(15, 5, 35);
inline constexpr Color BG_ACCENT = rgb(25, 10, 55);
inline constexpr Color NEON_PINK = rgb(255, 0, 255);
inline constexpr Color NEON_CYAN = rgb(0, 255, 255);
inline constexpr Color NEON_PURPLE = rgb(150, 0, 255);
inline constexpr Color NEON_GREEN = rgb(0, 255, 128);
inline constexpr Color NEON_ORANGE = rgb(255, 128, 0);
inline constexpr Color NEON_YELLOW = rgb(255, 255, 0);
inline constexpr Color GLOW_COLOR = rgb(100, 200, 255);
inline constexpr Color PANEL_BG = rgb(10, 5, 25);
inline constexpr Color PANEL_BORDER = rgb(255, 0, 255);
inline constexpr Color PANEL_GLOW = rgb(150, 0, 150);
inline constexpr Color HOLOGRAM_BG = rgb(0, 20, 40);
inline constexpr Color TEXT_NEON = rgb(0, 255, 255);
inline constexpr Color TEXT_HIGHLIGHT = rgb(255, 0, 255);
inline constexpr Color TEXT_DIM = rgb(100, 100, 150);
inline constexpr Color TEXT_MATRIX = rgb(0, 255, 0);
inline constexpr Color STATUS_ACTIVE = rgb(0, 255, 128);
inline constexpr Color STATUS_WARNING = rgb(255, 255, 0);
inline constexpr Color STATUS_ERROR = rgb(255, 0, 128);
inline constexpr Color STATUS_INFO = rgb(0, 200, 255);
inline constexpr Color TRACK_NEON_1 = rgb(255, 0, 255);
inline constexpr Color TRACK_NEON_2 = rgb(0, 255, 255);
inline constexpr Color TRACK_PULSE = rgb(255, 128, 255);
inline constexpr Color TRACK_CORE = rgb(200, 100, 255);
inline constexpr Color STATION_HOLO_IDLE = rgb(0, 100, 200);
inline constexpr Color STATION_HOLO_ACTIVE = rgb(0, 255, 200);
inline constexpr Color STATION_HOLO_GLOW = rgb(100, 200, 255);
inline constexpr Color VEHICLE_ENERGY = rgb(0, 255, 255);
inline constexpr Color VEHICLE_SHIELD = rgb(255, 0, 255);
inline constexpr Color VEHICLE_THRUST = rgb(255, 200, 0);
inline constexpr Color VEHICLE_TRAIL = rgb(100, 255, 255);
inline constexpr int TRANSPORT_LOG_ID_OFFSET = 100;
inline constexpr int DISPLAY_WIDTH = 1200;
inline constexpr int DISPLAY_HEIGHT = 800;
inline constexpr int LEFT_PANEL_WIDTH = 320;
inline constexpr int BOTTOM_BAR_HEIGHT = 80;
inline constexpr int MAIN_AREA_X = LEFT_PANEL_WIDTH + 10;
inline constexpr int MAIN_AREA_WIDTH = DISPLAY_WIDTH - LEFT_PANEL_WIDTH - 20;
inline constexpr int MAIN_AREA_HEIGHT = DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT - 10;
inline constexpr int MAX_PARTICLES = 30;

// 可覆盖的运行默认值；每次运行保存最终生效的配置。
struct SimulationConfig {
    bool patrol_mode = false;
    int unit_count = 3;
    double time_step = 0.05;
    double speed_factor = 10.0;
    std::uint32_t seed = 1;
    std::filesystem::path output_dir = "outputs";
    bool write_logs = true;
    bool console_logging = true;
};

} // 命名空间 warehouse
