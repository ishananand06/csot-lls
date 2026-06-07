#include "engine.hpp"
#include <iostream>
#include <dlfcn.h> 

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_strategy.so> <path_to_ticks.csv>\n"
                  << "Example: " << argv[0] << " ./build/spec_strategy.so data/synthetic_small.csv\n";
        return 1;
    }

    std::string strategy_path = argv[1];
    std::string market_data_path = argv[2];

    std::cout << "=== CSOT Dynamic Quant Replay Engine ===\n";
    std::cout << "Target Shared Object: " << strategy_path << "\n";
    std::cout << "Target Market Data  : " << market_data_path << "\n\n";

    csot::Engine engine;
    std::cout << "Parsing market data stream into RAM...\n";
    csot::LoadedMarketData data = engine.load_ticks(market_data_path);

    if (data.ticks.empty()) {
        std::cerr << "Error: No market data parsed. Execution halted.\n";
        return 1;
    }
    std::cout << "Loaded " << data.ticks.size() << " ticks successfully.\n\n";

    std::cout << "Loading dynamic shared object module...\n";
    void* handle = dlopen(strategy_path.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << "CRITICAL: Failed to load shared library. Error: " << dlerror() << "\n";
        return 1;
    }

    typedef csot::Strategy* (*create_strategy_t)();
    create_strategy_t create_strat_fn = (create_strategy_t)dlsym(handle, "create_strategy");
    
    char* error_msg = dlerror();
    if (error_msg != nullptr) {
        std::cerr << "CRITICAL: Could not find factory symbol 'create_strategy'. Error: " << error_msg << "\n";
        dlclose(handle);
        return 1;
    }

    std::cout << "Factory entry point resolved. Constructing strategy instance...\n";
    csot::Strategy* live_strategy = create_strat_fn();
    if (!live_strategy) {
        std::cerr << "CRITICAL: Strategy instance creation failed. Received null pointer.\n";
        dlclose(handle);
        return 1;
    }
    std::cout << "Strategy instance created successfully. Initializing strategy...\n";
    engine.run_backtest(data.ticks, live_strategy);

    std::cout << "\nShutting down strategy instance and releasing module handles...\n";
    delete live_strategy;
    dlclose(handle);

    std::cout << "=== Backtest Runner Session Finished Cleanly ===\n";
    return 0;
}