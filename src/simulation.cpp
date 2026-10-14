#include "warehouse/simulation.hpp"
#include "warehouse/logging.hpp"
#include <stdexcept>
#include <cmath>

namespace warehouse {

bool allOrdersCompleted(const SimulationContext& context) {
    if (context.config.patrol_mode) return false;
    if (context.cargo_orders.empty()) return true;

    for (const auto& order : context.cargo_orders) {
        if (order.progress != OrderProgress::FINISHED &&
            order.progress != OrderProgress::CANCELLED) {
            return false;
        }
    }
    return true;
}

void initializeSimulation(SimulationContext& context, int unitCount) {
    if (unitCount <= 0 || unitCount > 12) {
        throw std::invalid_argument("unit_count must be between 1 and 12");
    }
    if (!std::isfinite(context.config.time_step) || !std::isfinite(context.config.speed_factor) ||
        !(context.config.time_step > 0.0) || !(context.config.speed_factor > 0.0)) {
        throw std::invalid_argument("time_step and speed_factor must be positive");
    }
    closeSystemLogs(context);
    context.config.unit_count = unitCount;
    context.elapsed_time = 0.0;
    context.paused = false;
    context.complete = false;
    context.random_state = context.config.seed;
    context.search_log_times.clear();
    context.log_entries.clear();
    context.active_log_filename.clear();
    initialize_system_log(context);
    initializeWorkStations(context);
    initializeTransportUnits(context, unitCount);
    openSystemLogs(context);
    if (context.config.patrol_mode) {
        context.cargo_orders.clear();
        context.order_queues.clear();
    } else {
        loadCargoOrders(context);
        initializeScheduleMode(context);
    }
}

bool stepSimulation(SimulationContext& context) {
    if (context.complete) return true;
    if (context.paused) return false;
    if (!context.config.patrol_mode) {
        for (auto& pair : context.work_stations) {
            refresh_station_status(context, pair.second, context.cargo_orders, context.order_queues);
        }
        allocate_cargo_orders(context, context.transport_units, context.cargo_orders,
            context.work_stations);
    }
    for (int index = 0; index < static_cast<int>(context.transport_units.size()); ++index) {
        process_transport_dynamics(context, context.transport_units[index], index,
            context.transport_units, context.work_stations, context.cargo_orders);
    }
    context.elapsed_time += context.config.time_step;
    context.complete = !context.config.patrol_mode && allOrdersCompleted(context);
    return context.complete;
}

} // 命名空间 warehouse
