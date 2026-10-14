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

void updateTransportStatistics(SimulationContext& context, TransportUnit& unit) {
    if (unit.lastOperatingMode == unit.operatingMode) return;

    double timeDelta = context.elapsed_time - unit.lastStatusChangeTime;

    if (timeDelta > 0 && timeDelta < 10000) {

        if (unit.lastOperatingMode == TransportMode::STANDBY) {
            unit.totalIdleTime += timeDelta;
        }
        else if (unit.lastOperatingMode == TransportMode::COLLECTING_CARGO ||
                 unit.lastOperatingMode == TransportMode::DELIVERING_CARGO ||
                 unit.lastOperatingMode == TransportMode::NAVIGATING_TO_SOURCE ||
                 unit.lastOperatingMode == TransportMode::NAVIGATING_TO_DEST ||
                 unit.lastOperatingMode == TransportMode::SOURCE_WAIT ||
                 unit.lastOperatingMode == TransportMode::DEST_WAIT) {
            unit.totalCarryingTime += timeDelta;
        }
    }

    unit.lastStatusChangeTime = context.elapsed_time;
    unit.lastOperatingMode = unit.operatingMode;
}

void process_transport_dynamics(SimulationContext& context, TransportUnit& unit, int unitIndex,
                               std::vector<TransportUnit>& allUnits,
                               std::map<int, WorkStation>& stations,
                               std::vector<CargoOrder>& orders) {

    // 保留原接口中的车辆序号，当前运动规则仅按车辆 ID 区分。
    (void)unitIndex;
    updateTransportStatistics(context, unit);

    if (unit.operatingMode == TransportMode::COLLECTING_CARGO ||
        unit.operatingMode == TransportMode::DELIVERING_CARGO ||
        (context.config.patrol_mode && unit.operatingMode == TransportMode::PATROL_HALT)) {

        unit.operationCountdown -= context.config.time_step;
        if (unit.operationCountdown <= 0) {
            unit.operationCountdown = 0;

            if (unit.operatingMode == TransportMode::COLLECTING_CARGO) {
                if (unit.assignedOrderIndex != -1) {
                    CargoOrder& currentOrder = orders[unit.assignedOrderIndex];
                    currentOrder.collectionCompleteTime = context.elapsed_time;
                    record_station_status_change(context, unit.unitID, currentOrder.cargoCode,
                                               "Empty to Has Goods", true);

                    WorkStation& sourceStation = stations.at(currentOrder.sourceStationID);
                    sourceStation.currentMode = StationMode::VACANT;
                    sourceStation.lastModeChangeTime = context.elapsed_time;
                    record_station_status_change(context, sourceStation.stationID, currentOrder.cargoCode,
                                               "Has Goods to Empty");
                    sourceStation.activeOrderIndex = -1;

                    appendSystemLog(context, "Collection Complete: Order=" + std::to_string(currentOrder.orderID) +
                                   ", Cargo=" + currentOrder.cargoCode +
                                   ", Transport=" + std::to_string(unit.unitID));

                    unit.operatingMode = TransportMode::NAVIGATING_TO_DEST;
                    unit.hasDestination = true;
                    unit.targetPosition = stations.at(currentOrder.destStationID).trackPosition;
                } else {
                    unit.operatingMode = TransportMode::STANDBY;
                }
            }
            else if (unit.operatingMode == TransportMode::DELIVERING_CARGO) {
                if (unit.assignedOrderIndex != -1) {
                    CargoOrder& currentOrder = orders[unit.assignedOrderIndex];
                    currentOrder.deliveryCompleteTime = context.elapsed_time;
                    currentOrder.progress = OrderProgress::DELIVERED_PENDING_CLEAR;
                    record_station_status_change(context, unit.unitID, currentOrder.cargoCode,
                                               "Has Goods to Empty", true);

                    WorkStation& destStation = stations.at(currentOrder.destStationID);
                    destStation.currentMode = StationMode::CARGO_DELIVERED;
                    destStation.lastModeChangeTime = context.elapsed_time;
                    destStation.activeOrderIndex = unit.assignedOrderIndex;
                    record_station_status_change(context, destStation.stationID, currentOrder.cargoCode,
                                               "Empty to Has Goods");

                    appendSystemLog(context, "Delivery Complete: Order=" + std::to_string(currentOrder.orderID) +
                                   ", Cargo=" + currentOrder.cargoCode +
                                   ", Transport=" + std::to_string(unit.unitID));

                    unit.completedTaskCount++;
                    unit.operatingMode = TransportMode::STANDBY;
                    unit.assignedOrderIndex = -1;
                    unit.hasDestination = false;
                } else {
                    unit.operatingMode = TransportMode::STANDBY;
                }
            }
            else if (context.config.patrol_mode && unit.operatingMode == TransportMode::PATROL_HALT) {

                if (unit.operationCountdown < 900000.0) {
                    unit.operatingMode = TransportMode::PATROL_ACCELERATE;
                    unit.hasDestination = false;
                }

            }
        } else {
            unit.currentVelocity = 0;
            unit.activeAcceleration = 0;
            return;
        }
    }

    double desiredVelocity = 0;
    bool needsStop = false;

    if (unit.hasDestination) {
        double distToTarget = calculateForwardDistance(unit.currentPosition, unit.targetPosition);
        double maxSpeedHere = getSpeedLimitAtPosition(unit.currentPosition);
        double stoppingDistance = (unit.currentVelocity * unit.currentVelocity) /
                                 (2.0 * DECELERATION_RATE) + 5.0;
        double threshold1 = 5.0;
        double threshold2 = unit.currentVelocity * context.config.time_step * 1.1;
        double arrivalThreshold = (threshold1 > threshold2) ? threshold1 : threshold2;

        if (distToTarget <= arrivalThreshold) {
            unit.currentPosition = unit.targetPosition;
            unit.currentVelocity = 0;
            needsStop = true;

            if (!context.config.patrol_mode) {

                if (unit.assignedOrderIndex >= 0 && unit.assignedOrderIndex < (int)orders.size()) {
                    CargoOrder& currentOrder = orders[unit.assignedOrderIndex];

                    if (unit.operatingMode == TransportMode::NAVIGATING_TO_SOURCE ||
                        unit.operatingMode == TransportMode::SOURCE_WAIT) {
                        WorkStation& pickupStation = stations.at(currentOrder.sourceStationID);
                        if (pickupStation.currentMode == StationMode::CARGO_READY &&
                            pickupStation.activeOrderIndex == unit.assignedOrderIndex) {
                            unit.operatingMode = TransportMode::COLLECTING_CARGO;
                            unit.operationCountdown = CARGO_TRANSFER_DURATION;
                            pickupStation.currentMode = StationMode::TRANSPORT_ENGAGED;
                            pickupStation.lastModeChangeTime = context.elapsed_time;
                            unit.hasDestination = false;
                        } else {
                            unit.operatingMode = TransportMode::SOURCE_WAIT;
                        }
                    }
                    else if (unit.operatingMode == TransportMode::NAVIGATING_TO_DEST ||
                             unit.operatingMode == TransportMode::DEST_WAIT) {
                        WorkStation& dropoffStation = stations.at(currentOrder.destStationID);
                        if (dropoffStation.currentMode == StationMode::VACANT) {
                            unit.operatingMode = TransportMode::DELIVERING_CARGO;
                            unit.operationCountdown = CARGO_TRANSFER_DURATION;
                            dropoffStation.currentMode = StationMode::TRANSPORT_ENGAGED;
                            dropoffStation.lastModeChangeTime = context.elapsed_time;
                            unit.hasDestination = false;
                        } else {
                            unit.operatingMode = TransportMode::DEST_WAIT;
                        }
                    }
                } else {

                    if (unit.operatingMode == TransportMode::NAVIGATING_TO_SOURCE) {

                        unit.operatingMode = TransportMode::STANDBY;
                        unit.hasDestination = false;
                        unit.idleSearchCooldown = 0;

                        appendSystemLog(context, "Idle Transport T" + std::to_string(unit.unitID) +
                                      " arrived at station, resuming order search");
                    }
                }
            } else {

                if (unit.operatingMode == TransportMode::PATROL_DECELERATE ||
                    unit.operatingMode == TransportMode::PATROL_ACCELERATE ||
                    unit.operatingMode == TransportMode::PATROL_CRUISE) {

                    bool isFinalStop = false;
                    if (unit.patrolTargetStation != -1 && unit.patrolLapCompleted) {

                        if (stations.find(unit.patrolTargetStation) != stations.end()) {
                            double targetStationPos = stations.at(unit.patrolTargetStation).trackPosition;

                            double forwardDist = calculateForwardDistance(unit.currentPosition, targetStationPos);
                            double backwardDist = calculateForwardDistance(targetStationPos, unit.currentPosition);
                            double distToTarget = (forwardDist < backwardDist) ? forwardDist : backwardDist;
                            if (distToTarget < 10.0) {
                                isFinalStop = true;
                            }
                        }
                    }

                    unit.operatingMode = TransportMode::PATROL_HALT;
                    if (isFinalStop) {

                        unit.operationCountdown = 999999.0;
                        unit.patrolStopCount++;
                        appendSystemLog(context, "Unit " + std::to_string(unit.unitID) +
                                      " reached final stop at Station " +
                                      std::to_string(unit.patrolTargetStation));
                    } else {

                        unit.operationCountdown = (nextRandom(context) % 51 + 25) / 10.0;
                        unit.patrolStopCount++;
                    }
                    unit.hasDestination = false;
                }
            }
        } else if (distToTarget <= stoppingDistance) {
            desiredVelocity = 0;
        } else {
            desiredVelocity = maxSpeedHere;
        }
    } else {
        if (context.config.patrol_mode) {
            if (unit.operatingMode == TransportMode::STANDBY ||
                unit.operatingMode == TransportMode::PATROL_ACCELERATE) {
                unit.operatingMode = TransportMode::PATROL_ACCELERATE;

                double speed1 = unit.patrolTargetSpeed;
                double speed2 = getSpeedLimitAtPosition(unit.currentPosition);
                desiredVelocity = (speed1 < speed2) ? speed1 : speed2;
            } else if (unit.operatingMode == TransportMode::PATROL_CRUISE) {

                double speed1 = unit.patrolTargetSpeed;
                double speed2 = getSpeedLimitAtPosition(unit.currentPosition);
                desiredVelocity = (speed1 < speed2) ? speed1 : speed2;

                if (!unit.patrolLapCompleted && unit.patrolTargetStation != -1) {

                    if (unit.patrolDistanceTraveled > CIRCUIT_TOTAL_LENGTH * 0.7) {
                        unit.patrolLapCompleted = true;

                        if (stations.find(unit.patrolTargetStation) != stations.end()) {
                            unit.targetPosition = stations.at(unit.patrolTargetStation).trackPosition;
                            unit.hasDestination = true;
                            appendSystemLog(context, "Unit " + std::to_string(unit.unitID) +
                                          " completed lap (traveled " +
                                          std::to_string((int)unit.patrolDistanceTraveled) +
                                          "mm), heading to Station " +
                                          std::to_string(unit.patrolTargetStation));
                        }
                    }
                }
            } else if (unit.operatingMode == TransportMode::PATROL_DECELERATE) {

                double speed1 = unit.patrolTargetSpeed;
                double speed2 = getSpeedLimitAtPosition(unit.currentPosition);
                desiredVelocity = (speed1 < speed2) ? speed1 : speed2;
            } else {
                desiredVelocity = 0;
            }
        } else {
            desiredVelocity = 0;
        }
    }

    TransportUnit* leader = nullptr;
    double gapToLeader = CIRCUIT_TOTAL_LENGTH + 1.0;

    for (size_t i = 0; i < allUnits.size(); ++i) {
        if (allUnits[i].unitID == unit.unitID) continue;

        double otherRearPos = normalizeCircuitPosition(
            allUnits[i].currentPosition - TRANSPORT_UNIT_LENGTH / 2.0);
        double currentFrontPos = normalizeCircuitPosition(
            unit.currentPosition + TRANSPORT_UNIT_LENGTH / 2.0);
        double gap = calculateForwardDistance(currentFrontPos, otherRearPos);

        if (gap < gapToLeader) {
            gapToLeader = gap;
            leader = &allUnits[i];
        }
    }

    if (leader) {
        double stoppingDist = (unit.currentVelocity * unit.currentVelocity) /
                             (2.0 * DECELERATION_RATE);
        double safeFollowingDist = SAFETY_GAP_MINIMUM + stoppingDist +
                                  leader->currentVelocity * 0.1;

        if (gapToLeader < safeFollowingDist) {
            if (gapToLeader < SAFETY_GAP_MINIMUM + 5.0) {
                double maxSpeed = leader->currentVelocity - DECELERATION_RATE * context.config.time_step;
                if (maxSpeed < 0.0) maxSpeed = 0.0;
                if (desiredVelocity > maxSpeed) desiredVelocity = maxSpeed;
                if (gapToLeader < 1.0 && unit.currentVelocity > 0) {
                    desiredVelocity = 0;
                }
            } else {
                double speedFactor = gapToLeader / safeFollowingDist;
                double maxSpeed = leader->currentVelocity * speedFactor;
                if (desiredVelocity > maxSpeed) desiredVelocity = maxSpeed;
            }
        }
    }

    double speedLimit = getSpeedLimitAtPosition(unit.currentPosition);
    if (desiredVelocity > speedLimit) desiredVelocity = speedLimit;
    if (desiredVelocity < 0) desiredVelocity = 0;

    double oldSpeed = unit.currentVelocity;

    if (needsStop && unit.currentVelocity == 0 &&
        (!unit.hasDestination || std::abs(normalizeCircuitPosition(
            unit.currentPosition - unit.targetPosition)) < 1.0)) {
        unit.activeAcceleration = 0;
        unit.currentVelocity = 0;
    } else if (unit.currentVelocity < desiredVelocity - 1e-3) {
        unit.activeAcceleration = ACCELERATION_RATE;
        unit.currentVelocity += unit.activeAcceleration * context.config.time_step;
        if (unit.currentVelocity > desiredVelocity) unit.currentVelocity = desiredVelocity;
    } else if (unit.currentVelocity > desiredVelocity + 1e-3) {
        unit.activeAcceleration = -DECELERATION_RATE;
        unit.currentVelocity += unit.activeAcceleration * context.config.time_step;
        if (unit.currentVelocity < desiredVelocity) unit.currentVelocity = desiredVelocity;
        if (unit.currentVelocity < 0) unit.currentVelocity = 0;
    } else {
        unit.activeAcceleration = 0;
        unit.currentVelocity = desiredVelocity;
    }

    if (context.config.patrol_mode) {
        unit.patrolRunTime += context.config.time_step;

        bool isAccelerating = std::abs(unit.activeAcceleration) > 1e-3;
        bool accelChanged = (unit.activeAcceleration * unit.phaseAcceleration < -1e-3);
        bool speedBecameZero = unit.currentVelocity < 1e-3 && oldSpeed > 1e-3;

        if (unit.inAccelPhase) {
            if (!isAccelerating ||
                (speedBecameZero && isAccelerating) ||
                accelChanged) {
                recordPatrolPhase(context, unit);
            }
        }

        if (isAccelerating && !unit.inAccelPhase) {
            unit.inAccelPhase = true;
            unit.phaseStartTime = context.elapsed_time;
            unit.phaseStartVelocity = oldSpeed;
            unit.phaseAcceleration = unit.activeAcceleration;
        }

        if (unit.operatingMode != TransportMode::PATROL_HALT) {
            if (unit.activeAcceleration > 1e-3) {
                unit.operatingMode = TransportMode::PATROL_ACCELERATE;
            } else if (unit.activeAcceleration < -1e-3) {
                unit.operatingMode = TransportMode::PATROL_DECELERATE;
            } else if (unit.currentVelocity > 1e-3) {
                unit.operatingMode = TransportMode::PATROL_CRUISE;
            }
        }
    }

    double distanceMoved = unit.currentVelocity * context.config.time_step;
    if (context.config.patrol_mode && unit.patrolTargetStation != -1) {
        unit.patrolDistanceTraveled += distanceMoved;
    }
    unit.totalDistanceTraveled += std::abs(distanceMoved);
    unit.currentPosition += distanceMoved;
    unit.currentPosition = normalizeCircuitPosition(unit.currentPosition);
}

} // 命名空间 warehouse
