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

void allocate_cargo_orders(SimulationContext& context, std::vector<TransportUnit>& units,
                          std::vector<CargoOrder>& orders,
                          std::map<int, WorkStation>& stations) {
    if (context.config.patrol_mode) return;

    auto& lastSearchLogTime = context.search_log_times;

    for (size_t i = 0; i < units.size(); ++i) {
        TransportUnit& unit = units[i];
        if (unit.operatingMode == TransportMode::STANDBY) {

            if (unit.idleSearchCooldown > 0) {
                unit.idleSearchCooldown -= context.config.time_step;
                continue;
            }

            unit.idleSearchCooldown = 2.0;

            int bestOrderIndex = -1;
            double minDistance = CIRCUIT_TOTAL_LENGTH * 2.0;

            bool shouldLog = false;
            if (lastSearchLogTime.find(unit.unitID) == lastSearchLogTime.end() ||
                context.elapsed_time - lastSearchLogTime[unit.unitID] >= 5.0) {
                shouldLog = true;
                lastSearchLogTime[unit.unitID] = context.elapsed_time;
            }

            if (shouldLog) {
                appendSystemLog(context, "Idle Transport: T" + std::to_string(unit.unitID) +
                               ", Position=" + std::to_string(unit.currentPosition) +
                               "mm, Searching for orders");
            }

            for (int orderIdx = 0; orderIdx < static_cast<int>(orders.size()); ++orderIdx) {
                CargoOrder& order = orders[orderIdx];

                if (order.progress == OrderProgress::AWAITING_TRANSPORT) {
                    if (!stations.count(order.sourceStationID)) continue;

                    WorkStation& sourceStation = stations.at(order.sourceStationID);
                    if (sourceStation.currentMode == StationMode::CARGO_READY &&
                        sourceStation.activeOrderIndex == orderIdx) {

                        bool alreadyTargeted = false;
                        for (const auto& otherUnit : units) {
                            if (otherUnit.unitID != unit.unitID &&
                                otherUnit.assignedOrderIndex == orderIdx &&
                                (otherUnit.operatingMode == TransportMode::NAVIGATING_TO_SOURCE ||
                                 otherUnit.operatingMode == TransportMode::SOURCE_WAIT ||
                                 otherUnit.operatingMode == TransportMode::COLLECTING_CARGO)) {
                                alreadyTargeted = true;
                                break;
                            }
                        }
                        if (alreadyTargeted) continue;

                        double distance = calculateForwardDistance(unit.currentPosition,
                                                                 sourceStation.trackPosition);

                        if (shouldLog) {
                            appendSystemLog(context, "Available Order: ID=" + std::to_string(order.orderID) +
                                           ", Cargo=" + order.cargoCode +
                                           ", Station=" + std::to_string(order.sourceStationID) +
                                           ", Distance=" + std::to_string(distance) + "mm");
                        }

                        if (distance < minDistance) {
                            minDistance = distance;
                            bestOrderIndex = orderIdx;
                        }
                    }
                }
            }

            if (bestOrderIndex != -1) {
                unit.assignedOrderIndex = bestOrderIndex;
                CargoOrder& assignedOrder = orders[bestOrderIndex];
                assignedOrder.progress = OrderProgress::TRANSPORT_ASSIGNED;
                assignedOrder.assignedTransportID = unit.unitID;
                assignedOrder.transportHandling = true;

                unit.operatingMode = TransportMode::NAVIGATING_TO_SOURCE;
                unit.targetPosition = stations.at(assignedOrder.sourceStationID).trackPosition;
                unit.hasDestination = true;

                appendSystemLog(context, "Order Assigned: ID=" + std::to_string(assignedOrder.orderID) +
                               ", Cargo=" + assignedOrder.cargoCode +
                               ", Transport=" + std::to_string(unit.unitID) +
                               ", Source=" + std::to_string(assignedOrder.sourceStationID) +
                               ", Dest=" + std::to_string(assignedOrder.destStationID));
            } else {
                if (shouldLog) {
                    appendSystemLog(context, "Transport T" + std::to_string(unit.unitID) +
                                   " found no available orders");
                }

                if (!unit.hasDestination) {
                    std::vector<int> stationIDs;
                    for (const auto& pair : stations) {
                        stationIDs.push_back(pair.first);
                    }

                    if (!stationIDs.empty()) {
                        int randomStationID = stationIDs[nextRandom(context) % stationIDs.size()];
                        unit.targetPosition = stations.at(randomStationID).trackPosition;
                        unit.hasDestination = true;
                        unit.operatingMode = TransportMode::NAVIGATING_TO_SOURCE;

                        if (shouldLog) {
                            appendSystemLog(context, "Idle Transport T" + std::to_string(unit.unitID) +
                                           " moving to Station " + std::to_string(randomStationID));
                        }
                    }
                }
            }
        }
    }
}

} // 命名空间 warehouse
