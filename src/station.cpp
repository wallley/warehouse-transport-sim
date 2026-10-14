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

void refresh_station_status(SimulationContext& context, WorkStation& station, std::vector<CargoOrder>& orders,
                           std::map<int, std::deque<int>>& orderQueues) {

    double timeInPreviousMode = context.elapsed_time - station.lastModeChangeTime;
    if (station.lastModeChangeTime < context.elapsed_time - 1e-4) {
        if (station.currentMode == StationMode::VACANT) {
            station.idleTimeTotal += timeInPreviousMode;
        }
        if (station.currentMode == StationMode::CARGO_READY ||
            station.currentMode == StationMode::TRANSPORT_ENGAGED ||
            station.currentMode == StationMode::CARGO_DELIVERED) {
            station.occupiedTimeTotal += timeInPreviousMode;
        }
    }
    station.lastModeChangeTime = context.elapsed_time;

    if (station.operationTimer > 0) {
        station.operationTimer -= context.config.time_step;
        if (station.operationTimer <= 0) {
            station.operationTimer = 0;

            if (station.currentMode == StationMode::PREPARING_CARGO) {
                if (station.activeOrderIndex != -1 &&
                    station.activeOrderIndex < (int)orders.size()) {
                    if (orders[station.activeOrderIndex].progress == OrderProgress::STATION_PREPARING) {
                        CargoOrder& order = orders[station.activeOrderIndex];
                        order.progress = OrderProgress::AWAITING_TRANSPORT;
                        order.stationReadyTime = context.elapsed_time;
                        station.currentMode = StationMode::CARGO_READY;
                        record_station_status_change(context, station.stationID, order.cargoCode,
                                                   "Empty to Has Goods");

                        appendSystemLog(context, "Order Ready: ID=" + std::to_string(order.orderID) +
                                       ", Cargo=" + order.cargoCode +
                                       ", Station=" + std::to_string(station.stationID));
                    }
                } else {
                    station.currentMode = StationMode::VACANT;
                    station.activeOrderIndex = -1;
                }
            }
            else if (station.currentMode == StationMode::CLEARING_CARGO) {
                if (station.activeOrderIndex != -1 &&
                    station.activeOrderIndex < (int)orders.size()) {
                    if (orders[station.activeOrderIndex].progress == OrderProgress::DELIVERED_PENDING_CLEAR) {
                        CargoOrder& order = orders[station.activeOrderIndex];
                        order.progress = OrderProgress::FINISHED;
                        order.cargoRemovedTime = context.elapsed_time;
                        record_order_completion(context, order);

                        station.currentMode = StationMode::VACANT;
                        record_station_status_change(context, station.stationID, order.cargoCode,
                                                   "Has Goods to Empty");

                        appendSystemLog(context, "Order Complete: ID=" + std::to_string(order.orderID) +
                                       ", Cargo=" + order.cargoCode +
                                       ", Station=" + std::to_string(station.stationID));

                        station.activeOrderIndex = -1;
                    }
                } else {
                    station.currentMode = StationMode::VACANT;
                    station.activeOrderIndex = -1;
                }
            }
        }
    } else {

        if (station.currentMode == StationMode::VACANT) {
            if ((station.stationType == StationType::INBOUND_DOCK ||
                 station.stationType == StationType::STORAGE_OUTLET) &&
                orderQueues.count(station.stationID) &&
                !orderQueues[station.stationID].empty()) {

                int nextOrderIndex = orderQueues[station.stationID].front();

                if (nextOrderIndex >= 0 && nextOrderIndex < (int)orders.size() &&
                    station.activeOrderIndex == -1) {

                    CargoOrder& orderToPrepare = orders[nextOrderIndex];
                    if (orderToPrepare.progress == OrderProgress::WAITING &&
                        orderToPrepare.assignedTransportID == -1) {

                        orderQueues[station.stationID].pop_front();
                        station.activeOrderIndex = nextOrderIndex;

                        orderToPrepare.progress = OrderProgress::STATION_PREPARING;
                        station.currentMode = StationMode::PREPARING_CARGO;

                        if (station.stationType == StationType::INBOUND_DOCK) {
                            station.operationTimer = MANUAL_INBOUND_DURATION;
                        } else {
                            station.operationTimer = AUTOMATED_STORAGE_OUTBOUND_DURATION;
                        }
                    } else if (orderToPrepare.progress == OrderProgress::FINISHED) {
                        orderQueues[station.stationID].pop_front();
                    }
                }
            }
        }
        else if (station.currentMode == StationMode::CARGO_DELIVERED) {
            if (station.stationType == StationType::STORAGE_INLET ||
                station.stationType == StationType::OUTBOUND_DOCK) {
                if (station.activeOrderIndex != -1 &&
                    station.activeOrderIndex < (int)orders.size() &&
                    orders[station.activeOrderIndex].progress == OrderProgress::DELIVERED_PENDING_CLEAR) {

                    station.currentMode = StationMode::CLEARING_CARGO;
                    if (station.stationType == StationType::STORAGE_INLET) {
                        station.operationTimer = AUTOMATED_STORAGE_INBOUND_DURATION;
                    } else {
                        station.operationTimer = MANUAL_OUTBOUND_DURATION;
                    }
                }
            }
        }
    }
}

} // 命名空间 warehouse
