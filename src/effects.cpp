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

void initializeParticles(SimulationContext& context) {
    context.view.track_particles.clear();
    context.view.station_particles.clear();
    context.view.vehicle_trails.clear();

    if (context.view.enable_particles) {

        for (int i = 0; i < MAX_PARTICLES; ++i) {
            Particle p;
            p.x = nextRandom(context) % DISPLAY_WIDTH;
            p.y = nextRandom(context) % DISPLAY_HEIGHT;
            p.vx = (nextRandom(context) % 100 - 50) / 20.0;
            p.vy = (nextRandom(context) % 100 - 50) / 20.0;
            p.life = nextRandom(context) % 100 / 100.0;
            p.maxLife = 3.0 + nextRandom(context) % 100 / 50.0;
            p.color = (nextRandom(context) % 2) ? TRACK_NEON_1 : TRACK_NEON_2;
            p.size = 2;
            p.active = true;
            context.view.track_particles.push_back(p);
        }
    }

    if (context.view.enable_matrix_rain) {

        context.view.matrix_rain.clear();
        for (int x = 0; x < LEFT_PANEL_WIDTH; x += 30) {
            MatrixColumn col;
            col.y = -(nextRandom(context) % DISPLAY_HEIGHT);
            col.speed = 1 + nextRandom(context) % 3;
            col.brightness = 100 + nextRandom(context) % 150;
            for (int i = 0; i < 10; ++i) {
                col.chars.push_back(33 + nextRandom(context) % 94);
            }
            context.view.matrix_rain.push_back(col);
        }
    }
}

void initializeUIPanels(SimulationContext& context) {
    context.view.info_panels.clear();

    HoloPanel statusPanel;
    statusPanel.title = "SYSTEM STATUS";
    statusPanel.x = 10;
    statusPanel.y = 10;
    statusPanel.width = LEFT_PANEL_WIDTH - 20;
    statusPanel.height = 150;
    statusPanel.glowPhase = 0;
    statusPanel.collapsed = false;
    context.view.info_panels.push_back(statusPanel);

    HoloPanel transportPanel;
    transportPanel.title = "TRANSPORT UNITS";
    transportPanel.x = 10;
    transportPanel.y = 170;
    transportPanel.width = LEFT_PANEL_WIDTH - 20;
    transportPanel.height = 200;
    transportPanel.glowPhase = 0.5;
    transportPanel.collapsed = false;
    context.view.info_panels.push_back(transportPanel);

    HoloPanel orderPanel;
    orderPanel.title = "CARGO ORDERS";
    orderPanel.x = 10;
    orderPanel.y = 380;
    orderPanel.width = LEFT_PANEL_WIDTH - 20;
    orderPanel.height = 180;
    orderPanel.glowPhase = 1.0;
    orderPanel.collapsed = false;
    context.view.info_panels.push_back(orderPanel);

    HoloPanel logPanel;
    logPanel.title = "SYSTEM LOG";
    logPanel.x = 10;
    logPanel.y = 570;
    logPanel.width = LEFT_PANEL_WIDTH - 20;
    logPanel.height = DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT - 580;
    logPanel.glowPhase = 1.5;
    logPanel.collapsed = false;
    context.view.info_panels.push_back(logPanel);
}

void updateParticles(SimulationContext& context) {
    context.view.particle_phase += 0.02;

    for (auto& p : context.view.track_particles) {
        if (p.active) {
            p.x += p.vx;
            p.y += p.vy;
            p.life += 0.02;

            if (p.life > p.maxLife) {
                p.x = nextRandom(context) % DISPLAY_WIDTH;
                p.y = nextRandom(context) % DISPLAY_HEIGHT;
                p.life = 0;
                p.vx = (nextRandom(context) % 100 - 50) / 20.0;
                p.vy = (nextRandom(context) % 100 - 50) / 20.0;
            }

            if (p.x < 0) p.x = DISPLAY_WIDTH;
            if (p.x > DISPLAY_WIDTH) p.x = 0;
            if (p.y < 0) p.y = DISPLAY_HEIGHT;
            if (p.y > DISPLAY_HEIGHT) p.y = 0;
        }
    }
}

void updateMatrixRain(SimulationContext& context) {
    for (auto& col : context.view.matrix_rain) {
        col.y += col.speed;

        if (col.y > DISPLAY_HEIGHT) {
            col.y = -(nextRandom(context) % 200);
            col.speed = 2 + nextRandom(context) % 4;
            col.brightness = 50 + nextRandom(context) % 200;

            for (size_t i = 0; i < col.chars.size(); ++i) {
                col.chars[i] = 33 + nextRandom(context) % 94;
            }
        }
    }
}

void renderParticles(SimulationContext& context) {

    for (const auto& p : context.view.track_particles) {
        if (p.active) {
            double alpha = (1.0 - p.life / p.maxLife);
            int brightness = (int)(255 * alpha);

            Color particleColor = rgb(
                (p.color >> 16 & 0xFF) * brightness / 255,
                (p.color >> 8 & 0xFF) * brightness / 255,
                (p.color & 0xFF) * brightness / 255
            );

            setfillcolor(particleColor);
            solidcircle((int)p.x, (int)p.y, p.size);
        }
    }
}

} // 命名空间 warehouse
