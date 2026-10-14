#pragma once

#include "warehouse/model.hpp"
#include <map>
#include <deque>

namespace warehouse {

struct SimulationContext;

struct UIControl {
    int posX, posY, sizeW, sizeH;
    std::string label;
    char shortcut;
    bool active;

    UIControl(int x, int y, int w, int h, const std::string& lbl, char key, bool act = true)
        : posX(x), posY(y), sizeW(w), sizeH(h), label(lbl), shortcut(key), active(act) {}

    bool isHovered(int mx, int my) const {
        return mx >= posX && mx <= posX + sizeW && my >= posY && my <= posY + sizeH;
    }
};

class StationVisualizer {
private:
    struct VisualizationConfig {
        double displayOffset;
        double displayAngle;
        bool showConnector;
        Color connectorColor;
        int connectorStyle;
    };

    std::map<int, VisualizationConfig> stationConfigs;
    bool configLoaded;

public:
    StationVisualizer() : configLoaded(false) {}

    void loadConfiguration() {
        if (configLoaded) return;

        VisualizationConfig upperConfig = {
            1000.0,
            -MATH_PI/2,
            true,
            NEON_CYAN,
            2
        };

        for (int id = 13; id <= 18; ++id) {
            stationConfigs[id] = upperConfig;
        }

        VisualizationConfig lowerConfig = {
            1000.0,
            MATH_PI/2,
            true,
            NEON_PURPLE,
            2
        };

        for (int id = 1; id <= 12; ++id) {
            stationConfigs[id] = lowerConfig;
        }

        configLoaded = true;
    }

    void calculateStationDisplayPos(SimulationContext& context, const WorkStation& station, int& displayX, int& displayY);
    void renderStationConnectors(SimulationContext& context, const std::map<int, WorkStation>& stations);
};

struct Particle {
    double x, y;
    double vx, vy;
    double life;
    double maxLife;
    Color color;
    int size;
    bool active;
};

struct TrailPoint {
    double x, y;
    double intensity;
    Color color;
};

struct HoloPanel {
    std::string title;
    std::vector<std::string> lines;
    int x, y, width, height;
    double glowPhase;
    bool collapsed;
};

struct MatrixColumn {
    std::vector<char> chars;
    int y;
    int speed;
    int brightness;
};

// 图形表现状态归属于当前实例，不参与任务分配或运动约束。
struct ViewState {
    double render_scale = 1.0;
    double render_offset_x = 0.0;
    double render_offset_y = 0.0;
    double track_animation_phase = 0.0;
    double particle_phase = 0.0;
    double current_fps = 0.0;
    bool enable_particles = false;
    bool enable_matrix_rain = false;
    bool enable_glow_effects = true;
    bool enable_animations = true;
    int frame_skip = 2;
    int animation_frame = 0;
    StationVisualizer station_visualizer;
    std::vector<Particle> track_particles;
    std::vector<Particle> station_particles;
    std::map<int, std::deque<TrailPoint>> vehicle_trails;
    std::vector<HoloPanel> info_panels;
    std::vector<MatrixColumn> matrix_rain;
};

} // 命名空间 warehouse
