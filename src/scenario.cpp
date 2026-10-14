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

const std::string embedded_order_data =
"nan\t1\tTP001\tIncoming\t16\t1\n"
"nan\t2\tTP002\tIncoming\t16\t3\n"
"nan\t3\tTP003\tIncoming\t16\t5\n"
"nan\t4\tTP004\tIncoming\t16\t7\n"
"nan\t5\tTP005\tIncoming\t16\t9\n"
"nan\t6\tTP006\tIncoming\t16\t11\n"
"nan\t7\tTP007\tIncoming\t16\t1\n"
"nan\t8\tTP008\tIncoming\t16\t3\n"
"nan\t9\tTP009\tIncoming\t16\t5\n"
"nan\t10\tTP010\tIncoming\t16\t7\n"
"nan\t11\tTP011\tIncoming\t16\t9\n"
"nan\t12\tTP012\tIncoming\t16\t11\n"
"nan\t13\tTP013\tIncoming\t16\t1\n"
"nan\t14\tTP014\tIncoming\t16\t3\n"
"nan\t15\tTP015\tIncoming\t16\t5\n"
"nan\t16\tTP016\tIncoming\t16\t7\n"
"nan\t17\tTP017\tIncoming\t16\t9\n"
"nan\t18\tTP018\tIncoming\t16\t11\n"
"nan\t19\tTP019\tIncoming\t17\t1\n"
"nan\t20\tTP020\tIncoming\t17\t3\n"
"nan\t21\tTP021\tIncoming\t17\t5\n"
"nan\t22\tTP022\tIncoming\t17\t7\n"
"nan\t23\tTP023\tIncoming\t17\t9\n"
"nan\t24\tTP024\tIncoming\t17\t11\n"
"nan\t25\tTP025\tIncoming\t17\t1\n"
"nan\t26\tTP026\tIncoming\t17\t3\n"
"nan\t27\tTP027\tIncoming\t17\t5\n"
"nan\t28\tTP028\tIncoming\t17\t7\n"
"nan\t29\tTP029\tIncoming\t17\t9\n"
"nan\t30\tTP030\tIncoming\t17\t11\n"
"nan\t31\tTP031\tIncoming\t17\t1\n"
"nan\t32\tTP032\tIncoming\t17\t3\n"
"nan\t33\tTP033\tIncoming\t17\t5\n"
"nan\t34\tTP034\tIncoming\t17\t7\n"
"nan\t35\tTP035\tIncoming\t17\t9\n"
"nan\t36\tTP036\tIncoming\t17\t11\n"
"nan\t37\tTP037\tIncoming\t18\t1\n"
"nan\t38\tTP038\tIncoming\t18\t3\n"
"nan\t39\tTP039\tIncoming\t18\t5\n"
"nan\t40\tTP040\tIncoming\t18\t7\n"
"nan\t41\tTP041\tIncoming\t18\t9\n"
"nan\t42\tTP042\tIncoming\t18\t11\n"
"nan\t43\tTP043\tIncoming\t18\t1\n"
"nan\t44\tTP044\tIncoming\t18\t3\n"
"nan\t45\tTP045\tIncoming\t18\t5\n"
"nan\t46\tTP046\tIncoming\t18\t7\n"
"nan\t47\tTP047\tIncoming\t18\t9\n"
"nan\t48\tTP048\tIncoming\t18\t11\n"
"nan\t49\tTP049\tIncoming\t18\t1\n"
"nan\t50\tTP050\tIncoming\t18\t3\n"
"nan\t51\tTP051\tIncoming\t18\t5\n"
"nan\t52\tTP052\tIncoming\t18\t7\n"
"nan\t53\tTP053\tIncoming\t18\t9\n"
"nan\t54\tTP054\tIncoming\t18\t11\n"
"nan\t55\tTP055\tOutgoing\t2\t13\n"
"nan\t56\tTP056\tOutgoing\t2\t14\n"
"nan\t57\tTP057\tOutgoing\t2\t15\n"
"nan\t58\tTP058\tOutgoing\t2\t13\n"
"nan\t59\tTP059\tOutgoing\t2\t14\n"
"nan\t60\tTP060\tOutgoing\t2\t15\n"
"nan\t61\tTP061\tOutgoing\t2\t13\n"
"nan\t62\tTP062\tOutgoing\t2\t14\n"
"nan\t63\tTP063\tOutgoing\t2\t15\n"
"nan\t64\tTP064\tOutgoing\t4\t13\n"
"nan\t65\tTP065\tOutgoing\t4\t14\n"
"nan\t66\tTP066\tOutgoing\t4\t15\n"
"nan\t67\tTP067\tOutgoing\t4\t13\n"
"nan\t68\tTP068\tOutgoing\t4\t14\n"
"nan\t69\tTP069\tOutgoing\t4\t15\n"
"nan\t70\tTP070\tOutgoing\t4\t13\n"
"nan\t71\tTP071\tOutgoing\t4\t14\n"
"nan\t72\tTP072\tOutgoing\t4\t15\n"
"nan\t73\tTP073\tOutgoing\t6\t13\n"
"nan\t74\tTP074\tOutgoing\t6\t14\n"
"nan\t75\tTP075\tOutgoing\t6\t15\n"
"nan\t76\tTP076\tOutgoing\t6\t13\n"
"nan\t77\tTP077\tOutgoing\t6\t14\n"
"nan\t78\tTP078\tOutgoing\t6\t15\n"
"nan\t79\tTP079\tOutgoing\t6\t13\n"
"nan\t80\tTP080\tOutgoing\t6\t14\n"
"nan\t81\tTP081\tOutgoing\t6\t15\n"
"nan\t82\tTP082\tOutgoing\t8\t13\n"
"nan\t83\tTP083\tOutgoing\t8\t14\n"
"nan\t84\tTP084\tOutgoing\t8\t15\n"
"nan\t85\tTP085\tOutgoing\t8\t13\n"
"nan\t86\tTP086\tOutgoing\t8\t14\n"
"nan\t87\tTP087\tOutgoing\t8\t15\n"
"nan\t88\tTP088\tOutgoing\t8\t13\n"
"nan\t89\tTP089\tOutgoing\t8\t14\n"
"nan\t90\tTP090\tOutgoing\t8\t15\n"
"nan\t91\tTP091\tOutgoing\t10\t13\n"
"nan\t92\tTP092\tOutgoing\t10\t14\n"
"nan\t93\tTP093\tOutgoing\t10\t15\n"
"nan\t94\tTP094\tOutgoing\t10\t13\n"
"nan\t95\tTP095\tOutgoing\t10\t14\n"
"nan\t96\tTP096\tOutgoing\t10\t15\n"
"nan\t97\tTP097\tOutgoing\t10\t13\n"
"nan\t98\tTP098\tOutgoing\t10\t14\n"
"nan\t99\tTP099\tOutgoing\t10\t15\n"
"nan\t100\tTP100\tOutgoing\t12\t13\n"
"nan\t101\tTP101\tOutgoing\t12\t14\n"
"nan\t102\tTP102\tOutgoing\t12\t15\n"
"nan\t103\tTP103\tOutgoing\t12\t13\n"
"nan\t104\tTP104\tOutgoing\t12\t14\n"
"nan\t105\tTP105\tOutgoing\t12\t15\n"
"nan\t106\tTP106\tOutgoing\t12\t13\n"
"nan\t107\tTP107\tOutgoing\t12\t14\n"
"nan\t108\tTP108\tOutgoing\t12\t15\n";

void initializeWorkStations(SimulationContext& context) {
    context.work_stations.clear();

    auto createStation = [&](int id, StationType type, double segmentPos,
                            const std::string& namePrefix, bool isUpperTrack) {
        WorkStation station;
        station.stationID = id;
        station.stationType = type;

        if (isUpperTrack) {
            station.trackPosition = normalizeCircuitPosition(UPPER_LINEAR_BEGIN + segmentPos);
        } else {
            station.trackPosition = normalizeCircuitPosition(LOWER_LINEAR_BEGIN + segmentPos);
        }

        station.currentMode = StationMode::VACANT;
        station.activeOrderIndex = -1;
        station.operationTimer = 0.0;
        station.idleTimeTotal = 0.0;
        station.occupiedTimeTotal = 0.0;
        station.lastModeChangeTime = 0.0;
        station.stationName = namePrefix + std::to_string(id);

        context.work_stations[id] = station;
    };

    appendSystemLog(context, "Initializing Work Stations");

    const double STATION_WIDTH = 1100.0;

    double currentPosLower = 3250.0;
    createStation(12, StationType::STORAGE_OUTLET, currentPosLower, "Storage-Out", false);

    currentPosLower += 1300.0 + STATION_WIDTH;
    createStation(11, StationType::STORAGE_INLET, currentPosLower, "Storage-In", false);

    currentPosLower += 2500.0 + STATION_WIDTH;
    createStation(10, StationType::STORAGE_OUTLET, currentPosLower, "Storage-Out", false);

    currentPosLower += 1300.0 + STATION_WIDTH;
    createStation(9, StationType::STORAGE_INLET, currentPosLower, "Storage-In", false);

    currentPosLower += 2500.0 + STATION_WIDTH;
    createStation(8, StationType::STORAGE_OUTLET, currentPosLower, "Storage-Out", false);

    currentPosLower += 1300.0 + STATION_WIDTH;
    createStation(7, StationType::STORAGE_INLET, currentPosLower, "Storage-In", false);

    currentPosLower += 2500.0 + STATION_WIDTH;
    createStation(6, StationType::STORAGE_OUTLET, currentPosLower, "Storage-Out", false);

    currentPosLower += 1300.0 + STATION_WIDTH;
    createStation(5, StationType::STORAGE_INLET, currentPosLower, "Storage-In", false);

    currentPosLower += 2500.0 + STATION_WIDTH;
    createStation(4, StationType::STORAGE_OUTLET, currentPosLower, "Storage-Out", false);

    currentPosLower += 1300.0 + STATION_WIDTH;
    createStation(3, StationType::STORAGE_INLET, currentPosLower, "Storage-In", false);

    currentPosLower += 2500.0 + STATION_WIDTH;
    createStation(2, StationType::STORAGE_OUTLET, currentPosLower, "Storage-Out", false);

    currentPosLower += 1300.0 + STATION_WIDTH;
    createStation(1, StationType::STORAGE_INLET, currentPosLower, "Storage-In", false);

    double station13Pos = LINEAR_SEGMENT_SIZE - 7450.0;
    createStation(13, StationType::OUTBOUND_DOCK, station13Pos, "Out-Dock", true);

    double station14Pos = station13Pos - (1900.0 + STATION_WIDTH);
    createStation(14, StationType::OUTBOUND_DOCK, station14Pos, "Out-Dock", true);

    double station15Pos = station14Pos - (1900.0 + STATION_WIDTH);
    createStation(15, StationType::OUTBOUND_DOCK, station15Pos, "Out-Dock", true);

    double station16Pos = station15Pos - (10900.0 + STATION_WIDTH);
    createStation(16, StationType::INBOUND_DOCK, station16Pos, "In-Dock", true);

    double station17Pos = station16Pos - (1900.0 + STATION_WIDTH);
    createStation(17, StationType::INBOUND_DOCK, station17Pos, "In-Dock", true);

    double station18Pos = station17Pos - (1900.0 + STATION_WIDTH);
    createStation(18, StationType::INBOUND_DOCK, station18Pos, "In-Dock", true);

    context.view.station_visualizer.loadConfiguration();
    appendSystemLog(context, "Work stations initialized: " + std::to_string(context.work_stations.size()));
}

void initializeTransportUnits(SimulationContext& context, int unitCount) {
    context.transport_units.clear();

    if (context.work_stations.find(15) == context.work_stations.end()) {
        appendSystemLog(context, "Error: Station 15 not found for transport unit initialization");
        return;
    }

    double station15Position = context.work_stations.at(15).trackPosition;

    Color unitColors[] = {
        rgb(255, 100, 100), rgb(100, 255, 100), rgb(100, 100, 255),
        rgb(255, 255, 100), rgb(255, 100, 255), rgb(100, 255, 255),
        rgb(200, 150, 100), rgb(150, 200, 250), rgb(200, 250, 150),
        rgb(250, 150, 200), rgb(255, 200, 150), rgb(200, 150, 255)
    };

    int patrolTargetStations[] = {13, 15, 18};

    for (int i = 0; i < unitCount; ++i) {
        TransportUnit unit;
        unit.unitID = i + 1;

        if (i == 0) {
            unit.currentPosition = station15Position;
        } else {
            double prevUnitPos = context.transport_units[i - 1].currentPosition;

            unit.currentPosition = normalizeCircuitPosition(
                prevUnitPos - TRANSPORT_UNIT_LENGTH - SAFETY_GAP_MINIMUM - 1000.0);
        }

        unit.currentVelocity = 0.0;
        unit.activeAcceleration = 0.0;
        unit.operatingMode = context.config.patrol_mode ?
            TransportMode::PATROL_ACCELERATE : TransportMode::STANDBY;

        if (context.config.patrol_mode) {

            double speedRatio = 0.5 + (nextRandom(context) % 41) / 100.0;
            unit.patrolTargetSpeed = LINEAR_VELOCITY_LIMIT * speedRatio;

            if (unitCount == 3 && i < 3) {
                unit.patrolTargetStation = patrolTargetStations[i];
                unit.patrolLapCompleted = false;
                unit.hasDestination = false;
                unit.patrolStartPosition = unit.currentPosition;
                unit.patrolDistanceTraveled = 0.0;
            }

            unit.operatingMode = TransportMode::PATROL_ACCELERATE;
        }

        unit.assignedOrderIndex = -1;
        unit.hasDestination = false;
        unit.operationCountdown = 0.0;
        unit.unitColor = unitColors[i % (sizeof(unitColors)/sizeof(Color))];

        unit.patrolRunTime = 0.0;
        unit.patrolStopCount = 0;
        unit.patrolMotionHistory.clear();
        unit.inAccelPhase = false;

        unit.lastStatusChangeTime = context.elapsed_time;
        unit.lastOperatingMode = unit.operatingMode;

        context.transport_units.push_back(unit);
    }

    if (context.config.patrol_mode && unitCount == 3) {
        appendSystemLog(context, "Patrol mode initialized: Unit 1->Station 13, Unit 2->Station 15, Unit 3->Station 18");
        appendSystemLog(context, "Track total length: " + std::to_string((int)CIRCUIT_TOTAL_LENGTH) + " mm");
        appendSystemLog(context, "Lap completion threshold: " + std::to_string((int)(CIRCUIT_TOTAL_LENGTH * 0.7)) + " mm");
    }
    appendSystemLog(context, "Transport units initialized: " + std::to_string(unitCount));
}

void loadCargoOrders(SimulationContext& context) {
    context.cargo_orders.clear();
    context.order_queues.clear();

    std::stringstream dataStream(embedded_order_data);
    std::string dataLine;
    int orderIndex = 0;

    while (std::getline(dataStream, dataLine)) {
        if (dataLine.empty() || dataLine.find("nan") == std::string::npos) {
            continue;
        }

        std::stringstream lineStream(dataLine);
        std::string nanField, idStr, cargoStr, typeStr, sourceStr, destStr;

        std::getline(lineStream, nanField, '\t');
        std::getline(lineStream, idStr, '\t');
        std::getline(lineStream, cargoStr, '\t');
        std::getline(lineStream, typeStr, '\t');
        std::getline(lineStream, sourceStr, '\t');
        std::getline(lineStream, destStr, '\t');

        if (idStr.empty()) continue;

        CargoOrder order;
        try {
            order.orderID = std::stoi(idStr);
            order.cargoCode = cargoStr;
            order.category = (typeStr == "Incoming") ?
                CargoCategory::INCOMING : CargoCategory::OUTGOING;
            order.sourceStationID = std::stoi(sourceStr);
            order.destStationID = std::stoi(destStr);
            order.progress = OrderProgress::WAITING;
            order.sequenceIndex = orderIndex;

            context.cargo_orders.push_back(order);
            context.order_queues[order.sourceStationID].push_back(orderIndex);
            orderIndex++;
        } catch (const std::exception& e) {
            appendSystemLog(context, "Error parsing order data: " + std::string(e.what()));
        }
    }

    appendSystemLog(context, "Loaded " + std::to_string(context.cargo_orders.size()) + " cargo orders");
}

void initializeScheduleMode(SimulationContext& context) {
    if (context.config.patrol_mode) return;

    for (auto& pair : context.work_stations) {
        WorkStation& station = pair.second;
        bool isSourceStation = (station.stationType == StationType::INBOUND_DOCK ||
                               station.stationType == StationType::STORAGE_OUTLET);

        if (isSourceStation && context.order_queues.count(station.stationID) &&
            !context.order_queues[station.stationID].empty()) {

            int firstOrderIndex = context.order_queues[station.stationID].front();
            context.order_queues[station.stationID].pop_front();

            station.activeOrderIndex = firstOrderIndex;
            context.cargo_orders[firstOrderIndex].progress = OrderProgress::AWAITING_TRANSPORT;
            context.cargo_orders[firstOrderIndex].stationReadyTime = 0.0;

            station.currentMode = StationMode::CARGO_READY;
            station.lastModeChangeTime = 0.0;

            record_station_status_change(context, station.stationID,
                context.cargo_orders[firstOrderIndex].cargoCode, "Empty to Has Goods");
        }
    }
}

} // 命名空间 warehouse
