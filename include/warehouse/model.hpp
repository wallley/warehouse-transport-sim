#pragma once

#include "warehouse/config.hpp"
#include <string>
#include <vector>

namespace warehouse {

enum class CargoCategory { INCOMING, OUTGOING, UNDEFINED };

enum class OrderProgress {
    WAITING,
    STATION_PREPARING,
    AWAITING_TRANSPORT,
    TRANSPORT_ASSIGNED,
    TRANSPORT_APPROACHING,
    TRANSPORT_READY,
    CARGO_LOADING,
    IN_TRANSIT,
    ARRIVAL_WAIT,
    CARGO_UNLOADING,
    DELIVERED_PENDING_CLEAR,
    FINISHED,
    CANCELLED
};

enum class StationType {
    INBOUND_DOCK,
    OUTBOUND_DOCK,
    STORAGE_INLET,
    STORAGE_OUTLET
};

enum class StationMode {
    VACANT,
    PREPARING_CARGO,
    CARGO_READY,
    TRANSPORT_ENGAGED,
    CARGO_DELIVERED,
    CLEARING_CARGO
};

enum class TransportMode {
    STANDBY,
    NAVIGATING_TO_SOURCE,
    SOURCE_WAIT,
    COLLECTING_CARGO,
    NAVIGATING_TO_DEST,
    DEST_WAIT,
    DELIVERING_CARGO,
    PATROL_CRUISE,
    PATROL_ACCELERATE,
    PATROL_DECELERATE,
    PATROL_HALT
};

struct MotionLog {
    double startTime, endTime;
    double startVelocity, endVelocity;
    double appliedAcceleration;
};

struct CargoOrder {
    int orderID;
    std::string cargoCode;
    CargoCategory category;
    int sourceStationID;
    int destStationID;
    OrderProgress progress;
    int sequenceIndex;

    double stationReadyTime;
    int assignedTransportID;
    double collectionCompleteTime;
    double deliveryCompleteTime;
    double cargoRemovedTime;
    bool transportHandling;

    CargoOrder() : orderID(-1), category(CargoCategory::UNDEFINED),
                   sourceStationID(-1), destStationID(-1),
                   progress(OrderProgress::WAITING), sequenceIndex(-1),
                   stationReadyTime(0), assignedTransportID(-1),
                   collectionCompleteTime(0), deliveryCompleteTime(0),
                   cargoRemovedTime(0), transportHandling(false) {}
};

struct WorkStation {
    int stationID;
    StationType stationType;
    double trackPosition;
    StationMode currentMode;
    int activeOrderIndex;
    double operationTimer;

    double idleTimeTotal;
    double occupiedTimeTotal;
    double lastModeChangeTime;
    std::string stationName;

    WorkStation() : stationID(-1), trackPosition(0),
                    currentMode(StationMode::VACANT),
                    activeOrderIndex(-1), operationTimer(0),
                    idleTimeTotal(0), occupiedTimeTotal(0),
                    lastModeChangeTime(0) {}
};

struct TransportUnit {
    int unitID;
    double currentPosition;
    double currentVelocity;
    double activeAcceleration;
    TransportMode operatingMode;
    int assignedOrderIndex;

    double targetPosition;
    bool hasDestination;
    double operationCountdown;

    double patrolRunTime;
    int patrolStopCount;
    std::vector<MotionLog> patrolMotionHistory;
    double phaseStartTime;
    double phaseStartVelocity;
    double phaseAcceleration;
    bool inAccelPhase;

    double patrolTargetSpeed;
    int patrolTargetStation;
    bool patrolLapCompleted;
    double patrolStartPosition;
    double patrolDistanceTraveled;

    double idleSearchCooldown;

    int completedTaskCount;
    double totalIdleTime;
    double totalCarryingTime;
    double totalDistanceTraveled;
    double lastStatusChangeTime;
    TransportMode lastOperatingMode;

    Color unitColor;

    TransportUnit() : unitID(-1), currentPosition(0), currentVelocity(0),
                      activeAcceleration(0), operatingMode(TransportMode::STANDBY),
                      assignedOrderIndex(-1), targetPosition(0), hasDestination(false),
                      operationCountdown(0), patrolRunTime(0), patrolStopCount(0),
                      phaseStartTime(0), phaseStartVelocity(0), phaseAcceleration(0),
                      inAccelPhase(false), patrolTargetSpeed(0), patrolTargetStation(-1),
                      patrolLapCompleted(false), patrolStartPosition(0),
                      patrolDistanceTraveled(0), idleSearchCooldown(0),
                      completedTaskCount(0), totalIdleTime(0), totalCarryingTime(0),
                      totalDistanceTraveled(0), lastStatusChangeTime(0),
                      lastOperatingMode(TransportMode::STANDBY), unitColor(rgb(100, 150, 255)) {}

    bool isCarryingCargo(bool patrolMode = false) const {
        if (patrolMode) return false;
        return operatingMode == TransportMode::COLLECTING_CARGO ||
               operatingMode == TransportMode::NAVIGATING_TO_DEST ||
               operatingMode == TransportMode::DEST_WAIT ||
               operatingMode == TransportMode::DELIVERING_CARGO;
    }
};

} // 命名空间 warehouse
