#pragma once

#include "warehouse/context.hpp"
#include "warehouse/geometry.hpp"

namespace warehouse {

void initializeWorkStations(SimulationContext& context);
void initializeTransportUnits(SimulationContext& context, int unitCount);
void loadCargoOrders(SimulationContext& context);
void initializeScheduleMode(SimulationContext& context);
void updateTransportStatistics(SimulationContext& context, TransportUnit& unit);
void process_transport_dynamics(SimulationContext& context, TransportUnit& unit, int unitIndex,
    std::vector<TransportUnit>& allUnits, std::map<int, WorkStation>& stations,
    std::vector<CargoOrder>& orders);
void refresh_station_status(SimulationContext& context, WorkStation& station,
    std::vector<CargoOrder>& orders, std::map<int, std::deque<int>>& orderQueues);
void allocate_cargo_orders(SimulationContext& context, std::vector<TransportUnit>& units,
    std::vector<CargoOrder>& orders, std::map<int, WorkStation>& stations);
bool allOrdersCompleted(const SimulationContext& context);

// 初始化时先重置时钟和状态，再打开日志与准备订单。
void initializeSimulation(SimulationContext& context, int unitCount);
// 执行一个原始固定时间步，返回调度任务是否全部结束。
bool stepSimulation(SimulationContext& context);

} // 命名空间 warehouse
