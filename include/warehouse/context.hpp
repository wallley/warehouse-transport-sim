#pragma once

#include "warehouse/config.hpp"
#include "warehouse/model.hpp"
#include "warehouse/view.hpp"
#include <fstream>
#include <map>
#include <deque>

namespace warehouse {

// 一次运行的可变数据均由上下文持有，支持实例隔离与重复初始化。
struct SimulationContext {
    explicit SimulationContext(SimulationConfig initialConfig = {});

    SimulationConfig config;
    double elapsed_time = 0.0;
    bool paused = false;
    bool complete = false;
    std::uint32_t random_state = 1;
    std::map<int, double> search_log_times;
    std::vector<CargoOrder> cargo_orders;
    std::map<int, WorkStation> work_stations;
    std::vector<TransportUnit> transport_units;
    std::map<int, std::deque<int>> order_queues;
    std::vector<std::string> log_entries;
    std::string active_log_filename;
    std::ofstream cargo_execution_log;
    std::ofstream station_status_log;
    std::ofstream system_debug_log;
    ViewState view;
};

// 复用原 MSVC rand 的序列，随机状态由实例保存。
int nextRandom(SimulationContext& context);

} // 命名空间 warehouse
