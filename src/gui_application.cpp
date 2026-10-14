#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <graphics.h>
#include <windows.h>
#include <conio.h>
#include "warehouse/gui.hpp"
#include "warehouse/run_config.hpp"
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
#include <locale.h>

namespace warehouse {

void renderSimulationFrame(SimulationContext& context, int transportCount) {
    const int animFrame = context.view.animation_frame;
    BeginBatchDraw();

    setfillcolor(BG_DARK);
    solidrectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    setfillcolor(BG_MID);
    solidrectangle(MAIN_AREA_X, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT);

    if (animFrame % 10 == 0) {
        setlinecolor(rgb(30, 40, 80));
        setlinestyle(PS_DOT, 1);
        for (int x = MAIN_AREA_X; x < DISPLAY_WIDTH; x += 150) {
            line(x, 0, x, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT);
        }
        for (int y = 0; y < DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT; y += 150) {
            line(MAIN_AREA_X, y, DISPLAY_WIDTH, y);
        }
    }

    if (context.view.enable_particles && (animFrame % context.view.frame_skip == 0)) {
        renderParticles(context);
    }

    renderTransportPath(context);
    renderWorkStations(context, context.work_stations, context.cargo_orders);
    renderTransportUnits(context, context.transport_units, context.cargo_orders);

    renderSystemInfo(context, transportCount);

    EndBatchDraw();
}

void executeSimulationCycle(SimulationContext& context, int transportCount) {
    initializeSimulation(context, transportCount);
    saveRuntimeConfig(context.config, 10800.0);
    initializeRenderingParameters(context);
    initializeParticles(context);
    initializeUIPanels(context);

    unsigned long lastFrameTime = GetTickCount();
    unsigned long simStepsPerFrame = 0;
    bool simulationComplete = false;

    int frameCount = 0;
    double currentFPS = 0.0;
    unsigned long fpsLastTime = GetTickCount();

    while (true) {
        unsigned long currentFrameTime = GetTickCount();
        unsigned long elapsedRealTime = currentFrameTime - lastFrameTime;

        simStepsPerFrame = 0;
        if (context.paused) {
            lastFrameTime = currentFrameTime;
        } else {
            double targetStepDuration = (context.config.time_step * 1000.0);
            if (context.config.speed_factor > 1e-3) {
                targetStepDuration /= context.config.speed_factor;
            } else {
                targetStepDuration = 0;
            }

            if (targetStepDuration < 1.0) {
                simStepsPerFrame = (context.config.speed_factor > 50) ? 20 : ((context.config.speed_factor > 10) ? 10 : 5);
                lastFrameTime = currentFrameTime;
            } else if (elapsedRealTime >= targetStepDuration) {
                simStepsPerFrame = 1;
                if (elapsedRealTime > targetStepDuration * 1.2 && targetStepDuration > 0) {
                    unsigned long maxSteps = 20UL;
                    unsigned long calculatedSteps = (unsigned long)(elapsedRealTime / targetStepDuration);
                    simStepsPerFrame = (calculatedSteps < maxSteps) ? calculatedSteps : maxSteps;
                }
                lastFrameTime = currentFrameTime;
            }
        }

        if (simStepsPerFrame > 0 && !simulationComplete) {
            for (unsigned long step = 0; step < simStepsPerFrame; ++step) {
                if (stepSimulation(context)) {
                    simulationComplete = true;
                    context.paused = true;
                    appendSystemLog(context, "All orders completed (" +
                        std::to_string(transportCount) + " units)");
                    generateScheduleReport(context);
                    break;
                }
            }
        }

        int& animFrame = context.view.animation_frame;
        animFrame++;

        if (context.view.enable_animations) {
            context.view.track_animation_phase += 0.02;
        }

        if (context.view.enable_particles && (animFrame % context.view.frame_skip == 0)) {
            updateParticles(context);
        }

        if (context.view.enable_matrix_rain && (animFrame % (context.view.frame_skip * 2) == 0)) {
            updateMatrixRain(context);
        }

        frameCount++;
        unsigned long fpsCurrentTime = GetTickCount();
        if (fpsCurrentTime - fpsLastTime >= 1000) {
            currentFPS = frameCount * 1000.0 / (fpsCurrentTime - fpsLastTime);
            context.view.current_fps = currentFPS;
            frameCount = 0;
            fpsLastTime = fpsCurrentTime;
        }

        renderSimulationFrame(context, transportCount);

        if (_kbhit()) {
            char key = _getch();
            if (key == 'q' || key == 'Q') {
                if (context.config.patrol_mode && !context.transport_units.empty()) {
                    for (auto& unit : context.transport_units) {
                        if (unit.inAccelPhase) recordPatrolPhase(context, unit);
                    }
                    generatePatrolReport(context);
                }
                closeSystemLogs(context);
                return;
            }
            if (key == 'p' || key == 'P') context.paused = !context.paused;
            if (key == '+' || key == '=') {
                double newSpeed = context.config.speed_factor * 1.5;
                context.config.speed_factor = (newSpeed > 1024.0) ? 1024.0 : newSpeed;
            }
            if (key == '-') {
                double newSpeed = context.config.speed_factor / 1.5;
                context.config.speed_factor = (newSpeed < 0.05) ? 0.05 : newSpeed;
            }

            if (key == '1') {
                context.view.enable_particles = !context.view.enable_particles;
                std::string msg = context.view.enable_particles ? "Particles: ON" : "Particles: OFF";
                appendSystemLog(context, msg);
            }
            if (key == '2') {
                context.view.enable_matrix_rain = !context.view.enable_matrix_rain;
                std::string msg = context.view.enable_matrix_rain ? "Matrix Rain: ON" : "Matrix Rain: OFF";
                appendSystemLog(context, msg);
            }
            if (key == '3') {
                context.view.enable_glow_effects = !context.view.enable_glow_effects;
                std::string msg = context.view.enable_glow_effects ? "Glow Effects: ON" : "Glow Effects: OFF";
                appendSystemLog(context, msg);
            }
            if (key == '4') {
                context.view.enable_animations = !context.view.enable_animations;
                std::string msg = context.view.enable_animations ? "Animations: ON" : "Animations: OFF";
                appendSystemLog(context, msg);
            }

            if (key == 'd' || key == 'D') {
                std::string debugFile = "SystemDebug_" +
                    std::to_string((int)context.elapsed_time) + "s.txt";
                std::ofstream debugOut;
                if (context.config.write_logs) {
                    debugOut.open(context.config.output_dir / debugFile);
                }
                if (debugOut.is_open()) {
                    debugOut << "=== System Debug Snapshot ===" << std::endl;
                    debugOut << "Time: " << std::fixed << std::setprecision(2)
                            << context.elapsed_time << "s" << std::endl;
                    debugOut << "Mode: " << (context.config.patrol_mode ? "Patrol" : "Schedule") << std::endl;
                    debugOut << "\n--- Transport Units ---" << std::endl;

                    for (const auto& unit : context.transport_units) {
                        debugOut << "Unit " << unit.unitID << ": "
                                << getTransportModeName(unit.operatingMode)
                                << ", Position: " << unit.currentPosition << "mm";
                        if (unit.assignedOrderIndex != -1) {
                            debugOut << ", Order: " << unit.assignedOrderIndex;
                        }
                        debugOut << std::endl;
                    }

                    debugOut << "\n--- Recent Log ---" << std::endl;
                    for (const auto& msg : context.log_entries) {
                        debugOut << msg << std::endl;
                    }

                    debugOut.close();
                    appendSystemLog(context, "Debug snapshot saved: " + debugFile);
                }
            }

            if (key == 'm' || key == 'M') {
                if (context.config.patrol_mode && !context.transport_units.empty()) {
                    for (auto& unit : context.transport_units) {
                        if (unit.inAccelPhase) recordPatrolPhase(context, unit);
                    }
                    generatePatrolReport(context);
                }
                context.config.patrol_mode = !context.config.patrol_mode;
                closeSystemLogs(context);
                return;
            }
        }

        if (simulationComplete && !context.paused) {
            context.paused = true;
        }

        if (!context.config.patrol_mode && context.elapsed_time > 3600 * 3 && !simulationComplete) {
            std::cout << "Warning: Simulation timeout (3 hours)" << std::endl;
            context.paused = true;
            simulationComplete = true;
            generateScheduleReport(context);
        }

        if (simStepsPerFrame == 0 && !context.paused) {
            Sleep(10);
        } else if (!context.paused) {
            Sleep(5);
        } else {
            Sleep(50);
        }
    }
}

int runGraphicalApplication(int argc, char** argv) {
    bool previewMode = false;
    std::filesystem::path previewPath;
    if (argc == 3 && std::string(argv[1]) == "--preview") {
        previewMode = true;
        previewPath = std::filesystem::path(argv[2]);
    } else if (argc != 1) {
        std::cerr << "Usage: warehouse_gui [--preview screenshot.png]\n";
        return 2;
    }
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, "");
    initgraph(DISPLAY_WIDTH, DISPLAY_HEIGHT, previewMode ? 0 : EW_SHOWCONSOLE);
    if (GetHWnd() == nullptr) {
        std::cerr << "Graphics initialization failed\n";
        return 1;
    }
    SetWindowText(GetHWnd(), TEXT("Warehouse Transport Simulator"));
    SimulationConfig config;
    config.seed = previewMode ? 1u : static_cast<std::uint32_t>(std::time(nullptr));
    config.write_logs = !previewMode;
    config.console_logging = !previewMode;
    SimulationContext context(config);
    try {
        if (previewMode) {
            initializeSimulation(context, 3);
            initializeRenderingParameters(context);
            initializeParticles(context);
            initializeUIPanels(context);
            while (context.elapsed_time < 20.0 && !context.complete) {
                stepSimulation(context);
            }
            renderSimulationFrame(context, 3);
            if (previewPath.has_parent_path()) {
                std::filesystem::create_directories(previewPath.parent_path());
            }
            saveimage(previewPath.string().c_str());
            closegraph();
            return std::filesystem::exists(previewPath) ? 0 : 1;
        }
        int currentUnitCount = 3;
        bool systemRunning = true;
        while (systemRunning) {
            const MenuAction action = displaySystemMenu(context, context.config.patrol_mode,
                currentUnitCount);
            switch (action) {
                case MenuAction::BEGIN:
                    executeSimulationCycle(context, context.config.patrol_mode ? 3 : currentUnitCount);
                    appendSystemLog(context, "Simulation ended, returning to menu");
                    break;
                case MenuAction::CHANGE_TASKS:
                    context.config.patrol_mode = !context.config.patrol_mode;
                    appendSystemLog(context, "Mode switched to: " +
                        std::string(context.config.patrol_mode ? "Patrol" : "Schedule"));
                    break;
                case MenuAction::CHANGE_CARS:
                    appendSystemLog(context, "Transport unit count changed to: " +
                        std::to_string(currentUnitCount));
                    break;
                case MenuAction::EXIT:
                    systemRunning = false;
                    appendSystemLog(context, "System shutdown initiated");
                    break;
            }
        }
        closeSystemLogs(context);
        closegraph();
        return 0;
    } catch (const std::exception& exception) {
        closeSystemLogs(context);
        closegraph();
        std::cerr << exception.what() << '\n';
        return 1;
    }
}

} // 命名空间 warehouse
