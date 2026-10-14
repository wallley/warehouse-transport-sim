#pragma once

#include "warehouse/context.hpp"

namespace warehouse {

enum class MenuAction { BEGIN, CHANGE_TASKS, CHANGE_CARS, EXIT };

void convertToScreenCoordinates(SimulationContext& context, double trackPos, int& screenX, int& screenY);
void initializeRenderingParameters(SimulationContext& context);
void initializeParticles(SimulationContext& context);
void initializeUIPanels(SimulationContext& context);
void renderTransportPath(SimulationContext& context);
void renderWorkStations(SimulationContext& context, const std::map<int, WorkStation>& stations,
    const std::vector<CargoOrder>& orders);
void renderTransportUnits(SimulationContext& context, const std::vector<TransportUnit>& units,
    const std::vector<CargoOrder>& orders);
void renderSystemInfo(SimulationContext& context, int unitCount);
void drawProgressCircle(int x, int y, int radius, double progress, Color color);
void updateParticles(SimulationContext& context);
void updateMatrixRain(SimulationContext& context);
void renderParticles(SimulationContext& context);
void renderMenuButton(const UIControl& control, bool isHovered = false);
MenuAction displaySystemMenu(SimulationContext& context, bool& patrolMode, int& unitCount);
void executeSimulationCycle(SimulationContext& context, int transportCount);
void renderSimulationFrame(SimulationContext& context, int transportCount);
int runGraphicalApplication(int argc, char** argv);

} // 命名空间 warehouse
