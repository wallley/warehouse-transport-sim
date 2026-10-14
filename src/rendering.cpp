#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <graphics.h>
#include <windows.h>
#include <conio.h>
#include "warehouse/gui.hpp"
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

void convertToScreenCoordinates(SimulationContext& context, double trackPos, int& screenX, int& screenY) {
    trackPos = normalizeCircuitPosition(trackPos);
    double worldX, worldY;

    if (trackPos >= UPPER_LINEAR_BEGIN && trackPos < UPPER_LINEAR_END) {
        worldX = (trackPos - UPPER_LINEAR_BEGIN) - LINEAR_SEGMENT_SIZE / 2.0;
        worldY = -ARC_SECTION_RADIUS;
    }
    else if (trackPos >= RIGHT_ARC_BEGIN && trackPos < RIGHT_ARC_END) {
        double angle = MATH_PI * (trackPos - RIGHT_ARC_BEGIN) / ARC_SECTION_LENGTH;
        worldX = LINEAR_SEGMENT_SIZE / 2.0 + ARC_SECTION_RADIUS * sin(angle);
        worldY = -ARC_SECTION_RADIUS * cos(angle);
    }
    else if (trackPos >= LOWER_LINEAR_BEGIN && trackPos < LOWER_LINEAR_END) {
        worldX = (LINEAR_SEGMENT_SIZE / 2.0) - (trackPos - LOWER_LINEAR_BEGIN);
        worldY = ARC_SECTION_RADIUS;
    }
    else {
        double angle = MATH_PI * (trackPos - LEFT_ARC_BEGIN) / ARC_SECTION_LENGTH;
        worldX = -LINEAR_SEGMENT_SIZE / 2.0 - ARC_SECTION_RADIUS * sin(angle);
        worldY = ARC_SECTION_RADIUS * cos(angle);
    }

    screenX = static_cast<int>(context.view.render_offset_x + worldX * context.view.render_scale);
    screenY = static_cast<int>(context.view.render_offset_y - worldY * context.view.render_scale);
}

void initializeRenderingParameters(SimulationContext& context) {
    double pathWorldWidth = LINEAR_SEGMENT_SIZE + 2 * ARC_SECTION_RADIUS;
    double pathWorldHeight = 2 * ARC_SECTION_RADIUS;

    double scaleX = static_cast<double>(MAIN_AREA_WIDTH) / pathWorldWidth;
    double scaleY = static_cast<double>(MAIN_AREA_HEIGHT) / pathWorldHeight;
    context.view.render_scale = (scaleX < scaleY ? scaleX : scaleY) * 0.85;

    context.view.render_offset_x = MAIN_AREA_X + (MAIN_AREA_WIDTH / 2.0);
    context.view.render_offset_y = (MAIN_AREA_HEIGHT / 2.0);
}

void renderTransportPath(SimulationContext& context) {

    context.view.track_animation_phase += 0.05;
    if (context.view.track_animation_phase > 2 * MATH_PI) context.view.track_animation_phase -= 2 * MATH_PI;

    setlinecolor(TRACK_CORE);
    setlinestyle(PS_SOLID, 12);

    double segmentSize = 200.0;
    int prevX, prevY;
    convertToScreenCoordinates(context, 0, prevX, prevY);

    for (double pos = segmentSize; pos <= CIRCUIT_TOTAL_LENGTH; pos += segmentSize) {
        int currX, currY;
        convertToScreenCoordinates(context, pos, currX, currY);
        line(prevX, prevY, currX, currY);
        prevX = currX;
        prevY = currY;
    }

    int firstX, firstY;
    convertToScreenCoordinates(context, 0, firstX, firstY);
    line(prevX, prevY, firstX, firstY);

    if (context.view.enable_glow_effects) {
        setlinecolor(TRACK_NEON_1);
        setlinestyle(PS_SOLID, 2);

        double keyPoints[] = {0, CIRCUIT_TOTAL_LENGTH * 0.25,
                             CIRCUIT_TOTAL_LENGTH * 0.5,
                             CIRCUIT_TOTAL_LENGTH * 0.75};

        for (int i = 0; i < 4; i++) {
            int x, y;
            convertToScreenCoordinates(context, keyPoints[i], x, y);

            setfillcolor(TRACK_NEON_2);
            solidcircle(x, y, 8);
            setlinecolor(TRACK_NEON_1);
            circle(x, y, 12);
        }
    }

    double markerSpacing = 5000.0;
    for (double pos = 0; pos < CIRCUIT_TOTAL_LENGTH; pos += markerSpacing) {
        int x, y;
        convertToScreenCoordinates(context, pos, x, y);

        double holoPulse = sin(context.view.track_animation_phase * 2 + pos / 2000.0);
        int holoSize = 10 + (int)(5 * holoPulse);

        setlinecolor(NEON_CYAN);
        setlinestyle(PS_SOLID, 2);
        circle(x, y, holoSize);

        setfillcolor(NEON_PURPLE);
        solidcircle(x, y, 5);

        for (int i = 0; i < 8; i++) {
            double angle = i * MATH_PI / 4 + context.view.track_animation_phase;
            int x2 = x + (int)(holoSize * 1.5 * cos(angle));
            int y2 = y + (int)(holoSize * 1.5 * sin(angle));
            setlinecolor(TRACK_PULSE);
            setlinestyle(PS_DOT, 1);
            line(x, y, x2, y2);
        }
    }

}

void StationVisualizer::calculateStationDisplayPos(SimulationContext& context, const WorkStation& station,
                                                  int& displayX, int& displayY) {
    if (!configLoaded) loadConfiguration();

    int trackX, trackY;
    convertToScreenCoordinates(context, station.trackPosition, trackX, trackY);

    auto it = stationConfigs.find(station.stationID);
    if (it == stationConfigs.end()) {
        displayX = trackX;
        displayY = trackY;
        return;
    }

    const VisualizationConfig& config = it->second;

    double offsetWorldX = config.displayOffset * cos(config.displayAngle);
    double offsetWorldY = config.displayOffset * sin(config.displayAngle);

    int offsetScreenX = static_cast<int>(offsetWorldX * context.view.render_scale);
    int offsetScreenY = static_cast<int>(-offsetWorldY * context.view.render_scale);

    displayX = trackX + offsetScreenX;
    displayY = trackY + offsetScreenY;
}

void StationVisualizer::renderStationConnectors(SimulationContext& context, const std::map<int, WorkStation>& stations) {
    if (!configLoaded) loadConfiguration();

    for (const auto& pair : stations) {
        const WorkStation& station = pair.second;

        auto it = stationConfigs.find(station.stationID);
        if (it == stationConfigs.end() || !it->second.showConnector) {
            continue;
        }

        const VisualizationConfig& config = it->second;

        int stationX, stationY, trackX, trackY;
        calculateStationDisplayPos(context, station, stationX, stationY);
        convertToScreenCoordinates(context, station.trackPosition, trackX, trackY);

        setlinecolor(config.connectorColor);
        setlinestyle(config.connectorStyle, 2);
        line(stationX, stationY, trackX, trackY);

        setfillcolor(GLOW_COLOR);
        solidcircle(trackX, trackY, 3);

        setlinestyle(PS_SOLID, 1);
    }
}

void renderWorkStations(SimulationContext& context, const std::map<int, WorkStation>& stations,
                       const std::vector<CargoOrder>& orders) {

    for (const auto& pair : stations) {
        const WorkStation& station = pair.second;
        int stX, stY, trackX, trackY;
        context.view.station_visualizer.calculateStationDisplayPos(context, station, stX, stY);
        convertToScreenCoordinates(context, station.trackPosition, trackX, trackY);

        double beamPhase = context.view.track_animation_phase * 2 + station.stationID * 0.5;
        for (int i = 0; i < 5; i++) {
            double t = i / 5.0 + fmod(beamPhase, 1.0);
            if (t > 1.0) t -= 1.0;

            int beamX = trackX + (int)((stX - trackX) * t);
            int beamY = trackY + (int)((stY - trackY) * t);

            Color beamColor = (station.stationType == StationType::INBOUND_DOCK ||
                                 station.stationType == StationType::OUTBOUND_DOCK) ?
                                 NEON_CYAN : NEON_PURPLE;
            setfillcolor(beamColor);
            solidcircle(beamX, beamY, 3 - i/2);
        }
    }

    for (const auto& pair : stations) {
        const WorkStation& station = pair.second;
        int stX, stY;
        context.view.station_visualizer.calculateStationDisplayPos(context, station, stX, stY);

        Color holoColor = STATION_HOLO_IDLE;
        Color glowColor = NEON_CYAN;
        bool isActive = false;

        switch(station.currentMode) {
            case StationMode::VACANT:
                holoColor = STATION_HOLO_IDLE;
                glowColor = NEON_CYAN;

                break;
            case StationMode::CARGO_READY:
            case StationMode::CARGO_DELIVERED:
                holoColor = STATION_HOLO_ACTIVE;
                glowColor = NEON_GREEN;
                isActive = true;

                break;
            case StationMode::TRANSPORT_ENGAGED:
                holoColor = NEON_YELLOW;
                glowColor = NEON_ORANGE;
                isActive = true;

                break;
            case StationMode::PREPARING_CARGO:
            case StationMode::CLEARING_CARGO:
                holoColor = NEON_PURPLE;
                glowColor = NEON_PINK;

                break;
        }

        int stationWidth = 22;
        int stationDepth = 60;

        double orientation = 0;
        int trackX, trackY;
        convertToScreenCoordinates(context, station.trackPosition, trackX, trackY);

        bool isUpperTrack = (station.trackPosition >= UPPER_LINEAR_BEGIN &&
                            station.trackPosition < UPPER_LINEAR_END);

        if (isUpperTrack) {

            orientation = -MATH_PI / 2;
        } else {

            orientation = MATH_PI / 2;
        }

        POINT stationRect[4];
        double cosAngle = cos(orientation);
        double sinAngle = sin(orientation);

        int offsetX = stX - trackX;
        int offsetY = stY - trackY;
        double offsetDist = sqrt(offsetX * offsetX + offsetY * offsetY);
        if (offsetDist > 0) {

            int targetOffset = stationDepth/2 + 8;
            offsetX = (int)(offsetX / offsetDist * targetOffset);
            offsetY = (int)(offsetY / offsetDist * targetOffset);
        }

        int centerX = trackX + offsetX;
        int centerY = trackY + offsetY;

        double halfWidth = stationWidth / 2.0;
        double halfDepth = stationDepth / 2.0;

        stationRect[0].x = centerX + (LONG)(halfDepth * cosAngle - halfWidth * sinAngle);
        stationRect[0].y = centerY + (LONG)(halfDepth * sinAngle + halfWidth * cosAngle);

        stationRect[1].x = centerX + (LONG)(halfDepth * cosAngle + halfWidth * sinAngle);
        stationRect[1].y = centerY + (LONG)(halfDepth * sinAngle - halfWidth * cosAngle);

        stationRect[2].x = centerX + (LONG)(-halfDepth * cosAngle + halfWidth * sinAngle);
        stationRect[2].y = centerY + (LONG)(-halfDepth * sinAngle - halfWidth * cosAngle);

        stationRect[3].x = centerX + (LONG)(-halfDepth * cosAngle - halfWidth * sinAngle);
        stationRect[3].y = centerY + (LONG)(-halfDepth * sinAngle + halfWidth * cosAngle);

        if (isActive) {
            for (int g = 2; g >= 0; g--) {
                setlinecolor(glowColor);
                setlinestyle(PS_SOLID, 1);
                POINT glowRect[4];
                for (int i = 0; i < 4; i++) {
                    glowRect[i].x = stationRect[i].x + (centerX - stationRect[i].x) * g / 10;
                    glowRect[i].y = stationRect[i].y + (centerY - stationRect[i].y) * g / 10;
                }
                drawpoly(4, (int*)glowRect);
            }
        }

        setfillcolor(BG_DARK);
        fillpolygon(stationRect, 4);

        setlinecolor(holoColor);
        setlinestyle(PS_SOLID, 2);
        drawpoly(4, (int*)stationRect);

        settextcolor(TEXT_NEON);
        setbkmode(TRANSPARENT);
        settextstyle(14, 0, "Consolas");
        char idStr[10];
        snprintf(idStr, sizeof(idStr), "S%02d", station.stationID);
        int textW = textwidth(idStr);
        outtextxy(centerX - textW/2, centerY - 7, idStr);

        if (station.activeOrderIndex != -1 &&
            station.activeOrderIndex < (int)orders.size()) {

            const CargoOrder& order = orders[station.activeOrderIndex];

            int panelY = centerY + 40;
            double holoPhase = context.view.track_animation_phase + station.stationID * 0.3;
            double floatOffset = 5 * sin(holoPhase * 2);
            panelY += (int)floatOffset;

            setfillcolor(HOLOGRAM_BG);
            solidrectangle(centerX - 40, panelY, centerX + 40, panelY + 50);

            setlinecolor(glowColor);
            setlinestyle(PS_SOLID, 2);
            rectangle(centerX - 40, panelY, centerX + 40, panelY + 50);

            for (int scan = panelY + 5; scan < panelY + 45; scan += 10) {
                setlinecolor(rgb(0, 100, 100));
                setlinestyle(PS_DOT, 1);
                line(centerX - 35, scan, centerX + 35, scan);
            }

            settextcolor(NEON_CYAN);
            settextstyle(14, 0, "Consolas");
            outtextxy(centerX - 35, panelY + 5, order.cargoCode.c_str());

            std::string status = getStationModeName(station.currentMode);
            settextcolor(glowColor);
            settextstyle(12, 0, "Consolas");
            int statusW = textwidth(status.c_str());
            outtextxy(centerX - statusW/2, panelY + 25, status.c_str());

            int barWidth = 60;
            int barX = centerX - barWidth/2;
            int barY = panelY + 40;

            setlinecolor(NEON_PURPLE);
            rectangle(barX, barY, barX + barWidth, barY + 5);

            double progress = 0;
            if (station.operationTimer > 0) {
                if (station.stationType == StationType::INBOUND_DOCK)
                    progress = 1.0 - (station.operationTimer / MANUAL_INBOUND_DURATION);
                else if (station.stationType == StationType::OUTBOUND_DOCK)
                    progress = 1.0 - (station.operationTimer / MANUAL_OUTBOUND_DURATION);
                else if (station.stationType == StationType::STORAGE_INLET)
                    progress = 1.0 - (station.operationTimer / AUTOMATED_STORAGE_INBOUND_DURATION);
                else
                    progress = 1.0 - (station.operationTimer / AUTOMATED_STORAGE_OUTBOUND_DURATION);
            }

            if (progress > 0) {
                setfillcolor(glowColor);
                solidrectangle(barX + 1, barY + 1,
                             barX + (int)(barWidth * progress) - 1, barY + 4);
            }
        }
    }
}

void renderTransportUnits(SimulationContext& context, const std::vector<TransportUnit>& units,
                         const std::vector<CargoOrder>& orders) {

    for (const auto& unit : units) {
        if (unit.currentVelocity > 1.0) {
            TrailPoint tp;
            int tpX, tpY;
            convertToScreenCoordinates(context, unit.currentPosition, tpX, tpY);
            tp.x = tpX;
            tp.y = tpY;
            tp.intensity = unit.currentVelocity / LINEAR_VELOCITY_LIMIT;
            tp.color = unit.isCarryingCargo(context.config.patrol_mode) ? VEHICLE_SHIELD : VEHICLE_ENERGY;

            if (context.view.vehicle_trails[unit.unitID].size() > 20) {
                context.view.vehicle_trails[unit.unitID].pop_front();
            }
            context.view.vehicle_trails[unit.unitID].push_back(tp);
        }
    }

    for (const auto& trail : context.view.vehicle_trails) {
        const auto& points = trail.second;
        for (size_t i = 1; i < points.size(); ++i) {
            double alpha = (double)i / points.size();
            int thickness = 1 + (int)(3 * alpha * points[i].intensity);

            setlinecolor(points[i].color);
            setlinestyle(PS_SOLID, thickness);
            line((int)points[i-1].x, (int)points[i-1].y,
                 (int)points[i].x, (int)points[i].y);
        }
    }

    for (const auto& unit : units) {
        int unitX, unitY;
        convertToScreenCoordinates(context, unit.currentPosition, unitX, unitY);

        Color energyColor = VEHICLE_ENERGY;
        Color shieldColor = VEHICLE_SHIELD;
        bool hasShield = unit.isCarryingCargo(context.config.patrol_mode);
        double engineGlow = unit.currentVelocity / LINEAR_VELOCITY_LIMIT;

        double orientation = 0;
        if (unit.currentPosition >= UPPER_LINEAR_BEGIN &&
            unit.currentPosition < UPPER_LINEAR_END) {
            orientation = 0;
        } else if (unit.currentPosition >= RIGHT_ARC_BEGIN &&
                   unit.currentPosition < RIGHT_ARC_END) {
            double progress = (unit.currentPosition - RIGHT_ARC_BEGIN) / ARC_SECTION_LENGTH;
            orientation = MATH_PI/2 * progress;
        } else if (unit.currentPosition >= LOWER_LINEAR_BEGIN &&
                   unit.currentPosition < LOWER_LINEAR_END) {
            orientation = MATH_PI;
        } else {
            double progress = (unit.currentPosition - LEFT_ARC_BEGIN) / ARC_SECTION_LENGTH;
            orientation = MATH_PI + MATH_PI/2 * progress;
        }

        if (hasShield) {
            double shieldPhase = context.view.track_animation_phase * 3 + unit.unitID;
            for (int layer = 2; layer >= 0; layer--) {
                int shieldSize = 25 + layer * 5 + (int)(3 * sin(shieldPhase));
                setlinecolor(shieldColor);
                setlinestyle(PS_SOLID, 2 - layer);
                circle(unitX, unitY, shieldSize);
            }
        }

        int unitLength = 40;
        int unitWidth = 16;

        POINT rect[4];
        double cosAngle = cos(orientation);
        double sinAngle = sin(orientation);

        double halfLength = unitLength / 2.0;
        double halfWidth = unitWidth / 2.0;

        rect[0].x = unitX + (LONG)(halfLength * cosAngle - halfWidth * sinAngle);
        rect[0].y = unitY + (LONG)(halfLength * sinAngle + halfWidth * cosAngle);

        rect[1].x = unitX + (LONG)(halfLength * cosAngle + halfWidth * sinAngle);
        rect[1].y = unitY + (LONG)(halfLength * sinAngle - halfWidth * cosAngle);

        rect[2].x = unitX + (LONG)(-halfLength * cosAngle + halfWidth * sinAngle);
        rect[2].y = unitY + (LONG)(-halfLength * sinAngle - halfWidth * cosAngle);

        rect[3].x = unitX + (LONG)(-halfLength * cosAngle - halfWidth * sinAngle);
        rect[3].y = unitY + (LONG)(-halfLength * sinAngle + halfWidth * cosAngle);

        if (engineGlow > 0.1) {
            setfillcolor(VEHICLE_THRUST);

            for (int i = -unitLength/2; i < unitLength/2; i += 4) {
                int glowX = unitX + (int)(i * cosAngle);
                int glowY = unitY + (int)(i * sinAngle);
                int glowSize = 3 + (int)(2 * engineGlow);
                solidcircle(glowX, glowY + 5, glowSize);
            }
        }

        setfillcolor(BG_DARK);
        fillpolygon(rect, 4);

        setlinecolor(energyColor);
        setlinestyle(PS_SOLID, 3);
        drawpoly(4, (int*)rect);

        int frontX = unitX + (int)(unitLength/2 * cosAngle);
        int frontY = unitY + (int)(unitLength/2 * sinAngle);
        int rearX = unitX - (int)(unitLength/2 * cosAngle);
        int rearY = unitY - (int)(unitLength/2 * sinAngle);

        setfillcolor(NEON_GREEN);
        solidcircle(frontX, frontY, 3);

        setfillcolor(NEON_PINK);
        solidcircle(rearX, rearY, 3);

        setfillcolor(energyColor);
        solidcircle(unitX, unitY, 5);

        settextcolor(TEXT_NEON);
        setbkmode(TRANSPARENT);
        settextstyle(12, 0, "Consolas");
        char unitIdStr[5];
        snprintf(unitIdStr, sizeof(unitIdStr), "T%d", unit.unitID);
        int textW = textwidth(unitIdStr);
        outtextxy(unitX - textW/2, unitY - 6, unitIdStr);

        if (!context.config.patrol_mode) {

            int panelY = unitY - 40;
            int panelX = unitX;

            std::string statusText = getTransportModeName(unit.operatingMode);
            std::string orderText = "";

            if (unit.assignedOrderIndex != -1 &&
                unit.assignedOrderIndex < (int)orders.size()) {
                const CargoOrder& order = orders[unit.assignedOrderIndex];
                orderText = "[" + order.cargoCode + "]";
            }

            setfillcolor(HOLOGRAM_BG);
            solidrectangle(panelX - 40, panelY - 15, panelX + 40, panelY + 5);

            setlinecolor(energyColor);
            setlinestyle(PS_SOLID, 1);
            rectangle(panelX - 40, panelY - 15, panelX + 40, panelY + 5);

            settextcolor(TEXT_NEON);
            settextstyle(11, 0, "Consolas");
            setbkmode(TRANSPARENT);
            int statusW = textwidth(statusText.c_str());
            outtextxy(panelX - statusW/2, panelY - 12, statusText.c_str());

            if (!orderText.empty()) {
                settextcolor(NEON_YELLOW);
                int orderW = textwidth(orderText.c_str());
                outtextxy(panelX - orderW/2, panelY - 2, orderText.c_str());
            }

            if (unit.isCarryingCargo(context.config.patrol_mode)) {

                double cargoPhase = context.view.track_animation_phase * 4;

                setlinecolor(NEON_ORANGE);
                setlinestyle(PS_SOLID, 2);
                rectangle(unitX - 20, unitY + 25, unitX + 20, unitY + 35);

                for (int i = 0; i < 5; i++) {
                    int lineX = unitX - 18 + i * 9;
                    int lineY1 = unitY + 27;
                    int lineY2 = unitY + 33;

                    if (fmod(cargoPhase + i * 0.2, 1.0) < 0.5) {
                        setlinecolor(NEON_YELLOW);
                        line(lineX, lineY1, lineX, lineY2);
                    }
                }

                settextcolor(NEON_ORANGE);
                settextstyle(10, 0, "Consolas");
                outtextxy(unitX - 18, unitY + 26, "CARGO");
            }
        }

        if (unit.hasDestination) {
            int targetX, targetY;
            convertToScreenCoordinates(context, unit.targetPosition, targetX, targetY);

            setlinecolor(NEON_CYAN);
            setlinestyle(PS_DOT, 1);
            line(unitX, unitY, targetX, targetY);

            setfillcolor(STATUS_INFO);
            solidcircle(targetX, targetY, 5);
        }
    }
}

void renderSystemInfo(SimulationContext& context, int unitCount) {
    (void)unitCount;
    char buffer[256];

    setbkmode(TRANSPARENT);

    setfillcolor(BG_DARK);
    solidrectangle(0, 0, LEFT_PANEL_WIDTH, DISPLAY_HEIGHT);

    setlinecolor(NEON_PURPLE);
    setlinestyle(PS_SOLID, 3);
    line(LEFT_PANEL_WIDTH - 1, 0, LEFT_PANEL_WIDTH - 1, DISPLAY_HEIGHT);

    if (context.view.enable_matrix_rain) {
        settextcolor(TEXT_MATRIX);
        settextstyle(10, 0, "Consolas");
        for (size_t i = 0; i < context.view.matrix_rain.size() && i < 10; ++i) {
            MatrixColumn& col = context.view.matrix_rain[i];

            for (size_t j = 0; j < col.chars.size(); ++j) {
                int charY = col.y + j * 20;
                if (charY >= 0 && charY < DISPLAY_HEIGHT) {
                    settextcolor(rgb(0, col.brightness, 0));
                    char str[2] = { col.chars[j], '\0' };
                    outtextxy(i * 30 + 5, charY, str);
                }
            }
        }
    }

    int panelY = 10;

    setfillcolor(LIGHTBLUE);
    solidrectangle(10, panelY, LEFT_PANEL_WIDTH - 10, panelY + 100);

    for (int g = 2; g >= 0; g--) {
        setlinecolor(rgb(255 - g * 50, 0, 255 - g * 50));
        setlinestyle(PS_SOLID, 3 - g);
        rectangle(10 - g, panelY - g, LEFT_PANEL_WIDTH - 10 + g, panelY + 100 + g);
    }

    settextcolor(NEON_PINK);
    settextstyle(18, 0, "Consolas");
    outtextxy(20, panelY + 5, "WAREHOUSE SIMULATION");

    settextcolor(NEON_CYAN);
    settextstyle(14, 0, "Consolas");
    snprintf(buffer, sizeof(buffer), "TIME: %.2fs", context.elapsed_time);
    outtextxy(20, panelY + 30, buffer);

    snprintf(buffer, sizeof(buffer), "SPEED: x%.1f", context.config.speed_factor);
    outtextxy(20, panelY + 50, buffer);

    snprintf(buffer, sizeof(buffer), "MODE: %s", context.config.patrol_mode ? "PATROL" : "SCHEDULE");
    settextcolor(context.config.patrol_mode ? NEON_ORANGE : NEON_GREEN);
    outtextxy(20, panelY + 70, buffer);

    panelY += 110;

    setfillcolor(HOLOGRAM_BG);
    solidrectangle(10, panelY, LEFT_PANEL_WIDTH - 10, panelY + 180);

    setlinecolor(NEON_CYAN);
    setlinestyle(PS_SOLID, 2);
    rectangle(10, panelY, LEFT_PANEL_WIDTH - 10, panelY + 180);

    settextcolor(NEON_CYAN);
    settextstyle(16, 0, "Consolas");
    outtextxy(20, panelY + 5, "TRANSPORT UNIT STATUS");

    int unitY = panelY + 30;
    for (size_t i = 0; i < context.transport_units.size(); ++i) {
        const auto& unit = context.transport_units[i];

        settextcolor(VEHICLE_ENERGY);
        settextstyle(12, 0, "Consolas");
        snprintf(buffer, sizeof(buffer), "H%d", unit.unitID);
        outtextxy(20, unitY, buffer);

        int barX = 50;
        int barWidth = LEFT_PANEL_WIDTH - 70;

        setlinecolor(PANEL_GLOW);
        rectangle(barX, unitY, barX + barWidth, unitY + 15);

        Color fillColor = TEXT_DIM;
        int fillAmount = barWidth / 4;

        if (unit.isCarryingCargo(context.config.patrol_mode)) {
            fillColor = VEHICLE_SHIELD;
            fillAmount = barWidth;
        } else if (unit.operatingMode != TransportMode::STANDBY) {
            fillColor = VEHICLE_ENERGY;
            fillAmount = barWidth * 3 / 4;
        }

        setfillcolor(fillColor);
        solidrectangle(barX + 1, unitY + 1, barX + fillAmount - 1, unitY + 14);

        if (unit.currentVelocity > 1.0) {
            int speedPercent = (int)(unit.currentVelocity / LINEAR_VELOCITY_LIMIT * 100);
            settextcolor(NEON_YELLOW);
            settextstyle(10, 0, "Consolas");
            snprintf(buffer, sizeof(buffer), "%d%%", speedPercent);
            outtextxy(barX + barWidth + 5, unitY + 2, buffer);
        }

        unitY += 22;
    }

    panelY += 190;

    if (!context.config.patrol_mode && !context.cargo_orders.empty()) {
        int ordersComplete = 0;
        int ordersActive = 0;
        int ordersPending = 0;
        int ordersReady = 0;

        for (const auto& order : context.cargo_orders) {
            if (order.progress == OrderProgress::FINISHED) ordersComplete++;
            else if (order.progress == OrderProgress::WAITING ||
                     order.progress == OrderProgress::STATION_PREPARING) ordersPending++;
            else if (order.progress == OrderProgress::AWAITING_TRANSPORT) ordersReady++;
            else ordersActive++;
        }

        setfillcolor(HOLOGRAM_BG);
        solidrectangle(10, panelY, LEFT_PANEL_WIDTH - 10, panelY + 140);

        setlinecolor(NEON_GREEN);
        setlinestyle(PS_SOLID, 2);
        rectangle(10, panelY, LEFT_PANEL_WIDTH - 10, panelY + 140);

        settextcolor(NEON_GREEN);
        settextstyle(16, 0, "Consolas");
        outtextxy(20, panelY + 5, "CARGO OPERATIONS");

        int circleY = panelY + 35;
        int circleSpacing = 60;

        int totalX = 40;
        drawProgressCircle(totalX, circleY, 20, 1.0, NEON_PURPLE);
        settextcolor(NEON_PURPLE);
        settextstyle(14, 0, "Consolas");
        snprintf(buffer, sizeof(buffer), "%d", (int)context.cargo_orders.size());
        int textW = textwidth(buffer);
        outtextxy(totalX - textW/2, circleY - 7, buffer);
        settextstyle(10, 0, "Consolas");
        outtextxy(totalX - 15, circleY + 25, "TOTAL");

        int completeX = totalX + circleSpacing;
        double completeProgress = context.cargo_orders.empty() ? 0 :
            (double)ordersComplete / context.cargo_orders.size();
        drawProgressCircle(completeX, circleY, 20, completeProgress, STATUS_ACTIVE);
        settextcolor(STATUS_ACTIVE);
        settextstyle(14, 0, "Consolas");
        snprintf(buffer, sizeof(buffer), "%d", ordersComplete);
        textW = textwidth(buffer);
        outtextxy(completeX - textW/2, circleY - 7, buffer);
        settextstyle(10, 0, "Consolas");
        outtextxy(completeX - 18, circleY + 25, "DONE");

        int activeX = completeX + circleSpacing;
        drawProgressCircle(activeX, circleY, 20, ordersActive > 0 ? 0.5 : 0, STATUS_WARNING);
        settextcolor(STATUS_WARNING);
        settextstyle(14, 0, "Consolas");
        snprintf(buffer, sizeof(buffer), "%d", ordersActive);
        textW = textwidth(buffer);
        outtextxy(activeX - textW/2, circleY - 7, buffer);
        settextstyle(10, 0, "Consolas");
        outtextxy(activeX - 20, circleY + 25, "ACTIVE");

        int readyX = activeX + circleSpacing;
        drawProgressCircle(readyX, circleY, 20, ordersReady > 0 ? 0.7 : 0, STATUS_INFO);
        settextcolor(STATUS_INFO);
        settextstyle(14, 0, "Consolas");
        snprintf(buffer, sizeof(buffer), "%d", ordersReady);
        textW = textwidth(buffer);
        outtextxy(readyX - textW/2, circleY - 7, buffer);
        settextstyle(10, 0, "Consolas");
        outtextxy(readyX - 18, circleY + 25, "READY");

        int barY = circleY + 50;
        setlinecolor(PANEL_GLOW);
        rectangle(20, barY, LEFT_PANEL_WIDTH - 20, barY + 10);

        if (!context.cargo_orders.empty()) {
            int barWidth = LEFT_PANEL_WIDTH - 40;
            int completeWidth = (int)(barWidth * completeProgress);

            setfillcolor(STATUS_ACTIVE);
            solidrectangle(21, barY + 1, 20 + completeWidth, barY + 9);

            settextcolor(NEON_CYAN);
            settextstyle(10, 0, "Consolas");
            snprintf(buffer, sizeof(buffer), "%.1f%% Complete", completeProgress * 100);
            outtextxy(20, barY + 15, buffer);
        }

        panelY += 150;
    }

    int logHeight = DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT - panelY - 10;
    setfillcolor(HOLOGRAM_BG);
    solidrectangle(10, panelY, LEFT_PANEL_WIDTH - 10, panelY + logHeight);

    setlinecolor(NEON_ORANGE);
    setlinestyle(PS_SOLID, 2);
    rectangle(10, panelY, LEFT_PANEL_WIDTH - 10, panelY + logHeight);

    double logPhase = context.view.track_animation_phase * 2;
    Color logColor = (sin(logPhase) > 0) ? NEON_ORANGE : STATUS_ERROR;
    settextcolor(logColor);
    settextstyle(16, 0, "Consolas");
    outtextxy(20, panelY + 5, "SYSTEM LOG");

    int logY = panelY + 30;
    settextstyle(11, 0, "Consolas");

    int maxMessages = (logHeight - 40) / 14;
    int logSize = static_cast<int>(context.log_entries.size());
    int startIdx = (logSize > maxMessages) ? (logSize - maxMessages) : 0;

    for (int i = static_cast<int>(context.log_entries.size()) - 1; i >= startIdx; i--) {
        if (i >= 0 && logY < panelY + logHeight - 15) {

            Color msgColor = TEXT_DIM;
            if (context.log_entries[i].find("Warning") != std::string::npos) {
                msgColor = STATUS_WARNING;
            } else if (context.log_entries[i].find("Complete") != std::string::npos ||
                      context.log_entries[i].find("Ready") != std::string::npos) {
                msgColor = STATUS_ACTIVE;
            } else if (context.log_entries[i].find("Error") != std::string::npos) {
                msgColor = STATUS_ERROR;
            } else if (context.log_entries[i].find("Assigned") != std::string::npos) {
                msgColor = STATUS_INFO;
            }

            std::string msg = context.log_entries[i];
            if (msg.length() > 35) {
                msg = msg.substr(0, 32) + "...";
            }

            setfillcolor(msgColor);
            solidcircle(25, logY + 6, 2);

            settextcolor(msgColor);
            outtextxy(35, logY, msg.c_str());

            logY += 14;
        }
    }

    setfillcolor(BG_MID);
    solidrectangle(0, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    setlinecolor(NEON_PINK);
    setlinestyle(PS_SOLID, 3);
    line(0, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT);

    int btnY = DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT + 10;
    int btnHeight = BOTTOM_BAR_HEIGHT - 20;

    struct ControlBtn {
        int x, width;
        std::string label;
        std::string key;
        Color color;
    };

    ControlBtn buttons[] = {
        {LEFT_PANEL_WIDTH + 20, 120, "PAUSE", "[P]", context.paused ? STATUS_ERROR : STATUS_ACTIVE},
        {LEFT_PANEL_WIDTH + 150, 150, "SPEED", "[+/-]", STATUS_INFO},
        {LEFT_PANEL_WIDTH + 310, 120, "MODE", "[M]", NEON_PURPLE},
        {LEFT_PANEL_WIDTH + 440, 140, "DEBUG", "[D]", NEON_ORANGE},
        {LEFT_PANEL_WIDTH + 590, 100, "EXIT", "[Q]", STATUS_ERROR}
    };

    for (const auto& btn : buttons) {

        setfillcolor(PANEL_BG);
        solidrectangle(btn.x, btnY, btn.x + btn.width, btnY + btnHeight);

        setlinecolor(btn.color);
        setlinestyle(PS_SOLID, 2);
        rectangle(btn.x, btnY, btn.x + btn.width, btnY + btnHeight);

        setlinecolor(btn.color);
        setlinestyle(PS_DOT, 1);
        line(btn.x + 5, btnY + 5, btn.x + btn.width - 5, btnY + 5);
        line(btn.x + 5, btnY + btnHeight - 5, btn.x + btn.width - 5, btnY + btnHeight - 5);

        settextcolor(btn.color);
        settextstyle(16, 0, "Consolas");
        int labelW = textwidth(btn.label.c_str());
        outtextxy(btn.x + (btn.width - labelW) / 2, btnY + 8, btn.label.c_str());

        settextcolor(TEXT_DIM);
        settextstyle(12, 0, "Consolas");
        int keyW = textwidth(btn.key.c_str());
        outtextxy(btn.x + (btn.width - keyW) / 2, btnY + 30, btn.key.c_str());
    }

    int perfX = LEFT_PANEL_WIDTH + 740;
    settextcolor(TEXT_DIM);
    settextstyle(11, 0, "Consolas");
    outtextxy(perfX, btnY + 5, "Performance:");

    settextstyle(10, 0, "Consolas");
    settextcolor(context.view.enable_particles ? STATUS_ACTIVE : TEXT_DIM);
    outtextxy(perfX, btnY + 20, "[1] Particles");

    settextcolor(context.view.enable_matrix_rain ? STATUS_ACTIVE : TEXT_DIM);
    outtextxy(perfX + 90, btnY + 20, "[2] Matrix");

    settextcolor(context.view.enable_glow_effects ? STATUS_ACTIVE : TEXT_DIM);
    outtextxy(perfX, btnY + 35, "[3] Glow");

    settextcolor(context.view.enable_animations ? STATUS_ACTIVE : TEXT_DIM);
    outtextxy(perfX + 90, btnY + 35, "[4] Anim");

    int statusX = DISPLAY_WIDTH - 200;
    settextcolor(TEXT_NEON);
    settextstyle(14, 0, "Consolas");

    snprintf(buffer, sizeof(buffer), "FPS: %.1f", context.view.current_fps);
    if (context.view.current_fps < 30) {
        settextcolor(STATUS_ERROR);
    } else if (context.view.current_fps < 50) {
        settextcolor(STATUS_WARNING);
    } else {
        settextcolor(STATUS_ACTIVE);
    }
    outtextxy(statusX, btnY + 10, buffer);

    settextcolor(context.paused ? STATUS_ERROR : STATUS_ACTIVE);
    outtextxy(statusX, btnY + 30, context.paused ? "[PAUSED]" : "[RUNNING]");
}

void drawProgressCircle(int x, int y, int radius, double progress, Color color) {

    setlinecolor(PANEL_GLOW);
    setlinestyle(PS_SOLID, 2);
    circle(x, y, radius);

    if (progress > 0) {
        setlinecolor(color);
        setlinestyle(PS_SOLID, 3);

        int segments = 36;
        int filledSegments = (int)(segments * progress);

        for (int i = 0; i < filledSegments; i++) {
            double angle1 = -MATH_PI/2 + (i * 2 * MATH_PI / segments);
            double angle2 = -MATH_PI/2 + ((i + 1) * 2 * MATH_PI / segments);

            int x1 = x + (int)(radius * cos(angle1));
            int y1 = y + (int)(radius * sin(angle1));
            int x2 = x + (int)(radius * cos(angle2));
            int y2 = y + (int)(radius * sin(angle2));

            line(x1, y1, x2, y2);
        }
    }
}

} // 命名空间 warehouse
