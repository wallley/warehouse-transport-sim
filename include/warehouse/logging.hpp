#pragma once

#include "warehouse/context.hpp"

namespace warehouse {

void appendSystemLog(SimulationContext& context, const std::string& entry);
void initialize_system_log(SimulationContext& context);
void openSystemLogs(SimulationContext& context);
void closeSystemLogs(SimulationContext& context);
void record_order_completion(SimulationContext& context, const CargoOrder& order);
void record_station_status_change(SimulationContext& context, int entityID,
    const std::string& cargoID, const std::string& statusChange, bool isTransport = false);
void recordPatrolPhase(SimulationContext& context, TransportUnit& unit);
std::string getCategoryName(CargoCategory category);
std::string getTransportModeName(TransportMode mode);
std::string getStationModeName(StationMode mode);
void generatePatrolReport(SimulationContext& context);
void generateScheduleReport(SimulationContext& context);

} // 命名空间 warehouse
