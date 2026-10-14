#include "warehouse/simulation.hpp"
#include "parity_snapshot.hpp"

int main(int argc, char** argv) {
    try {
        const parity::ProbeOptions options = parity::parseOptions(argc, argv);
        if (options.help) {
            parity::printHelp();
            return 0;
        }
        warehouse::SimulationConfig config;
        config.patrol_mode = options.mode == "patrol";
        config.unit_count = options.units;
        config.seed = options.seed;
        config.time_step = options.time_step;
        config.output_dir = options.output.parent_path();
        config.write_logs = false;
        config.console_logging = false;
        warehouse::SimulationContext context(config);
        warehouse::initializeSimulation(context, options.units);
        parity::runProbe(options, context.cargo_orders, context.transport_units,
                         context.work_stations, context.order_queues, context.elapsed_time,
                         [&context]() { return warehouse::stepSimulation(context); });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Parity probe: " << error.what() << '\n';
        return 2;
    }
}
