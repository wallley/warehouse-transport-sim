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

void appendSystemLog(SimulationContext& context, const std::string& entry) {
    std::stringstream logEntry;
    logEntry << std::fixed << std::setprecision(2)
             << "[" << context.elapsed_time << "s] " << entry;

    context.log_entries.push_back(logEntry.str());

    if (context.log_entries.size() > LOG_BUFFER_SIZE) {
        context.log_entries.erase(context.log_entries.begin());
    }

    if (context.config.console_logging) { std::cout << logEntry.str() << std::endl; }
    if (context.config.console_logging) { std::cout.flush(); }

    if (context.system_debug_log.is_open()) {
        context.system_debug_log << logEntry.str() << std::endl;
        context.system_debug_log.flush();
    }
}

void initialize_system_log(SimulationContext& context) {
    if (!context.config.write_logs) return;
    std::filesystem::create_directories(context.config.output_dir);

    time_t currentTime = time(nullptr);
    struct tm* timeInfo = localtime(&currentTime);
    char timeBuffer[30];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S", timeInfo);

    context.active_log_filename = std::string("SystemLog_") + timeBuffer + ".txt";

    context.system_debug_log.open(context.config.output_dir / context.active_log_filename);
    if (context.system_debug_log.is_open()) {
        context.system_debug_log << "=== Warehouse Transport System Log ===" << std::endl;
        context.system_debug_log << "Start Time: " << timeBuffer << std::endl;
        context.system_debug_log << "=== Begin Log ===" << std::endl;
        appendSystemLog(context, "System initialized successfully");
        appendSystemLog(context, "Log file: " + context.active_log_filename);
    }
}

std::string getCategoryName(CargoCategory cat) {
    return cat == CargoCategory::INCOMING ? "Incoming" :
           (cat == CargoCategory::OUTGOING ? "Outgoing" : "Undefined");
}

std::string getTransportModeName(TransportMode mode) {
    switch (mode) {
        case TransportMode::STANDBY: return "Standby";
        case TransportMode::NAVIGATING_TO_SOURCE: return "To Source";
        case TransportMode::SOURCE_WAIT: return "Wait Source";
        case TransportMode::COLLECTING_CARGO: return "Collecting";
        case TransportMode::NAVIGATING_TO_DEST: return "To Dest";
        case TransportMode::DEST_WAIT: return "Wait Dest";
        case TransportMode::DELIVERING_CARGO: return "Delivering";
        case TransportMode::PATROL_CRUISE: return "Patrol";
        case TransportMode::PATROL_ACCELERATE: return "Patrol-Acc";
        case TransportMode::PATROL_DECELERATE: return "Patrol-Dec";
        case TransportMode::PATROL_HALT: return "Patrol-Stop";
        default: return "Unknown";
    }
}

std::string getStationModeName(StationMode mode) {
    switch (mode) {
        case StationMode::VACANT: return "Vacant";
        case StationMode::PREPARING_CARGO: return "Preparing";
        case StationMode::CARGO_READY: return "Ready";
        case StationMode::TRANSPORT_ENGAGED: return "Engaged";
        case StationMode::CARGO_DELIVERED: return "Delivered";
        case StationMode::CLEARING_CARGO: return "Clearing";
        default: return "Unknown";
    }
}

void openSystemLogs(SimulationContext& context) {
    if (!context.config.write_logs) return;
    std::filesystem::create_directories(context.config.output_dir);

    std::string logSuffix = context.config.patrol_mode ? "_Patrol" :
        (std::to_string(context.transport_units.size()) + "units");
    std::string executionLogName = "TaskExeLog" + logSuffix + ".txt";
    std::string statusLogName = "DeviceStateLog" + logSuffix + ".txt";

    context.cargo_execution_log.open(context.config.output_dir / executionLogName);
    if (context.cargo_execution_log.is_open()) {
        context.cargo_execution_log << "任务编号 \t 物料编号 \t 任务类型 \t 起始设备 \t 目的设备 \t "
                           << "起始时间 \t 穿梭车ID \t 取货完成时间 \t 放货完成时间 \t 货物取走时间 \n ";
    }

    context.station_status_log.open(context.config.output_dir / statusLogName);
    if (context.station_status_log.is_open()) {
        context.station_status_log << "Time \t DeviceID \t Cargo ID  \t Status \n ";
    }
}

void closeSystemLogs(SimulationContext& context) {
    if (context.cargo_execution_log.is_open()) context.cargo_execution_log.close();
    if (context.station_status_log.is_open()) context.station_status_log.close();
    if (context.system_debug_log.is_open()) {
        context.system_debug_log << "=== System Log End ===" << std::endl;
        context.system_debug_log.close();
    }
}

void record_order_completion(SimulationContext& context, const CargoOrder& order) {
    if (!context.cargo_execution_log.is_open() || context.config.patrol_mode) return;

    context.cargo_execution_log << order.orderID << "\t\t"
                       << order.cargoCode << "\t\t"
                       << getCategoryName(order.category) << "\t\t"
                       << order.sourceStationID << "\t\t"
                       << order.destStationID << "\t\t\t"
                       << std::fixed << std::setprecision(2) << order.stationReadyTime << "\t\t"
                       << (order.assignedTransportID == -1 ? "N/A" : std::to_string(order.assignedTransportID)) << "\t\t\t\t"
                       << order.collectionCompleteTime << "\t\t\t"
                       << order.deliveryCompleteTime << "\t\t"
                       << order.cargoRemovedTime << "\n";
    context.cargo_execution_log.flush();
}

void record_station_status_change(SimulationContext& context, int entityID, const std::string& cargoID,
                                 const std::string& statusChange, bool isTransport) {
    if (!context.station_status_log.is_open() || context.config.patrol_mode) return;

    int logID = isTransport ? (entityID + TRANSPORT_LOG_ID_OFFSET) : entityID;
    context.station_status_log << std::fixed << std::setprecision(2) << context.elapsed_time << "\t\t"
                      << logID << "\t\t"
                      << (cargoID.empty() ? "N/A" : cargoID) << "\t\t"
                      << statusChange << "\n";
    context.station_status_log.flush();
}

void recordPatrolPhase(SimulationContext& context, TransportUnit& unit) {
    if (!context.config.patrol_mode || !unit.inAccelPhase) return;

    MotionLog record;
    record.startTime = unit.phaseStartTime;
    record.endTime = context.elapsed_time;
    record.startVelocity = unit.phaseStartVelocity;
    record.endVelocity = unit.currentVelocity;
    record.appliedAcceleration = unit.phaseAcceleration;

    unit.patrolMotionHistory.push_back(record);
    unit.inAccelPhase = false;
}

void generatePatrolReport(SimulationContext& context) {
    if (!context.config.write_logs) return;
    if (!context.config.patrol_mode) return;

    std::stringstream report;
    report << "\n\n==================================================" << std::endl;
    report << "            Patrol Mode Analysis Report" << std::endl;
    report << "==================================================" << std::endl;

    if (context.config.console_logging) { std::cout << report.str(); }

    std::ofstream reportFile(context.config.output_dir / "PatrolAnalysisReport.txt");
    if (reportFile.is_open()) {
        reportFile << "--- Patrol Mode Analysis ---" << std::endl;

        for (const auto& unit : context.transport_units) {
            std::stringstream unitInfo;
            unitInfo << "Transport Unit " << unit.unitID << ":" << std::endl;
            unitInfo << "  Target Speed: " << std::fixed << std::setprecision(2)
                    << unit.patrolTargetSpeed << " mm/s" << std::endl;
            unitInfo << "  Target Station: " << unit.patrolTargetStation << std::endl;
            unitInfo << "  Distance Traveled: " << std::fixed << std::setprecision(2)
                    << unit.patrolDistanceTraveled << " mm" << std::endl;
            unitInfo << "  Lap Completed: " << (unit.patrolLapCompleted ? "Yes" : "No") << std::endl;
            unitInfo << "  Total Runtime: " << std::fixed << std::setprecision(2)
                    << unit.patrolRunTime << " seconds" << std::endl;
            unitInfo << "  Stop Count: " << unit.patrolStopCount << std::endl;
            unitInfo << "  Final Status: " << (unit.operatingMode == TransportMode::PATROL_HALT ?
                                             "Stopped" : "Moving") << std::endl;
            unitInfo << "\n  Acceleration/Deceleration Records:" << std::endl;
            unitInfo << "  +==============+============+================+================+==============+================+" << std::endl;
            unitInfo << "  | Start Time(s)| End Time(s)| Start Speed    | End Speed      | Acceleration | Duration(s)    |" << std::endl;
            unitInfo << "  |              |            | (mm/s)         | (mm/s)         | (mm/s^2)     |                |" << std::endl;
            unitInfo << "  +--------------+------------+----------------+----------------+--------------+----------------+" << std::endl;

            if (unit.patrolMotionHistory.empty()) {
                unitInfo << "  |                    No acceleration/deceleration records                                      |" << std::endl;
            } else {
                for (const auto& record : unit.patrolMotionHistory) {
                    unitInfo << "  | " << std::setw(12) << std::fixed << std::setprecision(2) << record.startTime
                            << " | " << std::setw(10) << record.endTime
                            << " | " << std::setw(14) << record.startVelocity
                            << " | " << std::setw(14) << record.endVelocity
                            << " | " << std::setw(12) << record.appliedAcceleration
                            << " | " << std::setw(14) << (record.endTime - record.startTime) << " |" << std::endl;
                }
            }
            unitInfo << "  +==============+============+================+================+==============+================+" << std::endl;

            if (context.config.console_logging) { std::cout << unitInfo.str(); }
            reportFile << unitInfo.str();
        }

        reportFile.close();
        if (context.config.console_logging) { std::cout << "--------------------------------------------------" << std::endl; }
        if (context.config.console_logging) { std::cout << "Patrol analysis report saved to PatrolAnalysisReport.txt" << std::endl; }
    }
}

void generateScheduleReport(SimulationContext& context) {
    if (!context.config.write_logs) return;
    if (context.config.patrol_mode) return;
    if (context.transport_units.empty() || context.cargo_orders.empty()) return;

    std::string suffix = "_Schedule_" + std::to_string(context.transport_units.size()) + "units";
    std::ofstream reportFile(context.config.output_dir / ("SystemAnalysisReport" + suffix + ".txt"));

    if (!reportFile.is_open()) return;

    std::stringstream report;
    report << "\n\n==================================================" << std::endl;
    report << "     Schedule Mode Analysis (Units: " << context.transport_units.size() << ")" << std::endl;
    report << "==================================================" << std::endl;
    report << "Total completion time: " << std::fixed << std::setprecision(2)
           << context.elapsed_time << " seconds" << std::endl;
    report << "Transport units deployed: " << context.transport_units.size() << std::endl;
    report << "\nStation Performance Metrics:" << std::endl;
    report << "Station ID\tName\tIdle Time(s)\tOccupied Time(s)" << std::endl;
    report << "--------------------------------------------------" << std::endl;

    reportFile << "--- Schedule Mode Analysis Report ---" << std::endl;
    reportFile << "Total completion time: " << std::fixed << std::setprecision(2)
               << context.elapsed_time << " seconds" << std::endl;
    reportFile << "Transport units: " << context.transport_units.size() << std::endl;
    reportFile << "\nStation Metrics:\n";
    reportFile << "Station ID\tName\tIdle Time(s)\tOccupied Time(s)\n";

    if (context.config.console_logging) { std::cout << report.str(); }

    for (auto& pair : context.work_stations) {
        WorkStation& station = pair.second;
        double finalStateTime = context.elapsed_time - station.lastModeChangeTime;
        if (station.lastModeChangeTime < context.elapsed_time - 1e-4) {
            if (station.currentMode == StationMode::VACANT) {
                station.idleTimeTotal += finalStateTime;
            }
            if (station.currentMode == StationMode::CARGO_READY ||
                station.currentMode == StationMode::TRANSPORT_ENGAGED ||
                station.currentMode == StationMode::CARGO_DELIVERED) {
                station.occupiedTimeTotal += finalStateTime;
            }
        }

        reportFile << station.stationID << "\t" << station.stationName << "\t"
                  << std::fixed << std::setprecision(2) << station.idleTimeTotal << "\t"
                  << station.occupiedTimeTotal << "\n";

        if (context.config.console_logging) { std::cout << station.stationID << "\t" << station.stationName << "\t"
                 << std::fixed << std::setprecision(2) << station.idleTimeTotal << "\t"
                 << station.occupiedTimeTotal << std::endl; }
    }

    report.str("");
    report << "\n\nTransport Unit Performance Metrics:" << std::endl;
    report << "+========+================+=============+=================+================+==================+" << std::endl;
    report << "| Unit ID| Completed Tasks| Idle Time(s)| Carrying Time(s)| Total Distance | Avg Speed(mm/s)  |" << std::endl;
    report << "+--------+----------------+-------------+-----------------+----------------+------------------+" << std::endl;

    reportFile << "\n\nTransport Unit Metrics:\n";
    reportFile << "Unit ID\tCompleted Tasks\tIdle Time(s)\tCarrying Time(s)\tTotal Distance(mm)\tAvg Speed(mm/s)\n";

    if (context.config.console_logging) { std::cout << report.str(); }

    for (const auto& unit : context.transport_units) {

        double finalTimeDelta = context.elapsed_time - unit.lastStatusChangeTime;
        double finalIdleTime = unit.totalIdleTime;
        double finalCarryingTime = unit.totalCarryingTime;

        if (unit.operatingMode == TransportMode::STANDBY) {
            finalIdleTime += finalTimeDelta;
        }
        else if (unit.operatingMode == TransportMode::COLLECTING_CARGO ||
                 unit.operatingMode == TransportMode::DELIVERING_CARGO ||
                 unit.operatingMode == TransportMode::NAVIGATING_TO_SOURCE ||
                 unit.operatingMode == TransportMode::NAVIGATING_TO_DEST ||
                 unit.operatingMode == TransportMode::SOURCE_WAIT ||
                 unit.operatingMode == TransportMode::DEST_WAIT) {
            finalCarryingTime += finalTimeDelta;
        }

        double avgSpeed = (context.elapsed_time > 0) ?
                         (unit.totalDistanceTraveled / context.elapsed_time) : 0;

        if (context.config.console_logging) { std::cout << "| " << std::setw(6) << unit.unitID
                 << " | " << std::setw(14) << unit.completedTaskCount
                 << " | " << std::setw(11) << std::fixed << std::setprecision(2) << finalIdleTime
                 << " | " << std::setw(15) << finalCarryingTime
                 << " | " << std::setw(14) << std::setprecision(0) << unit.totalDistanceTraveled
                 << " | " << std::setw(16) << std::setprecision(2) << avgSpeed << " |" << std::endl; }

        reportFile << unit.unitID << "\t" << unit.completedTaskCount << "\t"
                  << std::fixed << std::setprecision(2) << finalIdleTime << "\t"
                  << finalCarryingTime << "\t"
                  << std::setprecision(0) << unit.totalDistanceTraveled << "\t"
                  << std::setprecision(2) << avgSpeed << "\n";
    }

    if (context.config.console_logging) { std::cout << "+========+================+=============+=================+================+==================+" << std::endl; }

    int totalCompletedTasks = 0;
    double totalIdleTimeAll = 0;
    double totalCarryingTimeAll = 0;
    double totalDistanceAll = 0;

    for (const auto& unit : context.transport_units) {
        totalCompletedTasks += unit.completedTaskCount;

        double finalTimeDelta = context.elapsed_time - unit.lastStatusChangeTime;
        if (unit.operatingMode == TransportMode::STANDBY) {
            totalIdleTimeAll += unit.totalIdleTime + finalTimeDelta;
        } else {
            totalIdleTimeAll += unit.totalIdleTime;
        }

        if (unit.operatingMode == TransportMode::COLLECTING_CARGO ||
            unit.operatingMode == TransportMode::DELIVERING_CARGO ||
            unit.operatingMode == TransportMode::NAVIGATING_TO_SOURCE ||
            unit.operatingMode == TransportMode::NAVIGATING_TO_DEST ||
            unit.operatingMode == TransportMode::SOURCE_WAIT ||
            unit.operatingMode == TransportMode::DEST_WAIT) {
            totalCarryingTimeAll += unit.totalCarryingTime + finalTimeDelta;
        } else {
            totalCarryingTimeAll += unit.totalCarryingTime;
        }

        totalDistanceAll += unit.totalDistanceTraveled;
    }

    if (context.config.console_logging) { std::cout << "\nSummary Statistics:" << std::endl; }
    if (context.config.console_logging) { std::cout << "Total Tasks Completed: " << totalCompletedTasks << std::endl; }
    if (context.config.console_logging) { std::cout << "Average Tasks per Unit: " << std::fixed << std::setprecision(2)
             << (double)totalCompletedTasks / context.transport_units.size() << std::endl; }
    if (context.config.console_logging) { std::cout << "Total Idle Time (all units): " << totalIdleTimeAll << " seconds" << std::endl; }
    if (context.config.console_logging) { std::cout << "Total Carrying Time (all units): " << totalCarryingTimeAll << " seconds" << std::endl; }
    if (context.config.console_logging) { std::cout << "Total Distance Traveled (all units): " << std::setprecision(0)
             << totalDistanceAll << " mm" << std::endl; }

    reportFile << "\nSummary:\n";
    reportFile << "Total Tasks Completed: " << totalCompletedTasks << "\n";
    reportFile << "Average Tasks per Unit: " << std::fixed << std::setprecision(2)
               << (double)totalCompletedTasks / context.transport_units.size() << "\n";
    reportFile << "Total Idle Time: " << totalIdleTimeAll << " seconds\n";
    reportFile << "Total Carrying Time: " << totalCarryingTimeAll << " seconds\n";
    reportFile << "Total Distance: " << std::setprecision(0) << totalDistanceAll << " mm\n";

    reportFile.close();
    if (context.config.console_logging) { std::cout << "--------------------------------------------------" << std::endl; }
    if (context.config.console_logging) { std::cout << "Analysis report saved to SystemAnalysisReport" << suffix << ".txt" << std::endl; }

    if (context.config.console_logging) { std::cout << "\nSystem Optimization Recommendations:" << std::endl; }
    if (context.config.console_logging) { std::cout << "1. Station Bottleneck Analysis: Review occupied vs idle times" << std::endl; }
    if (context.config.console_logging) { std::cout << "2. Transport Distribution: Check workload balance across units" << std::endl; }
    if (context.config.console_logging) { std::cout << "3. Path Optimization: Consider station placement for reduced travel" << std::endl; }
    if (context.config.console_logging) { std::cout << "4. Capacity Planning: Evaluate if more transport units would help" << std::endl; }
}

} // 命名空间 warehouse
