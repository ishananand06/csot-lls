#include "engine.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <benchmark/benchmark.h>
#include <x86intrin.h>
#include <thread>

namespace csot {

LoadedMarketData Engine::load_ticks(const std::string& path) {
    LoadedMarketData data;
    
    std::ifstream file(path, std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open market data file at " << path << "\n";
        return data;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        return data;
    }

    const char* ptr = buffer.data();
    const char* end = ptr + size;

    while (ptr < end && *ptr != '\n') ptr++;
    if (ptr < end) ptr++;

    data.ticks.reserve(10000);

    while (ptr < end) {
        if (*ptr == '\n' || *ptr == '\r' || *ptr == '\0') {
            ptr++;
            continue;
        }

        uint64_t timestamp = 0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            timestamp = timestamp * 10 + (*ptr - '0');
            ptr++;
        }
        ptr++;

        const char* sym_start = ptr;
        while (ptr < end && *ptr != ',') ptr++;
        std::string_view sym_view(sym_start, ptr - sym_start);
        ptr++;

        double bid_px = 0.0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            bid_px = bid_px * 10.0 + (*ptr - '0');
            ptr++;
        }
        if (ptr < end && *ptr == '.') {
            ptr++;
            double factor = 0.1;
            while (ptr < end && *ptr >= '0' && *ptr <= '9') {
                bid_px += (*ptr - '0') * factor;
                factor *= 0.1;
                ptr++;
            }
        }
        ptr++;

        double ask_px = 0.0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            ask_px = ask_px * 10.0 + (*ptr - '0');
            ptr++;
        }
        if (ptr < end && *ptr == '.') {
            ptr++;
            double factor = 0.1;
            while (ptr < end && *ptr >= '0' && *ptr <= '9') {
                ask_px += (*ptr - '0') * factor;
                factor *= 0.1;
                ptr++;
            }
        }
        ptr++;

        uint32_t bid_qty = 0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            bid_qty = bid_qty * 10 + (*ptr - '0');
            ptr++;
        }
        ptr++;

        uint32_t ask_qty = 0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            ask_qty = ask_qty * 10 + (*ptr - '0');
            ptr++;
        }

        std::string_view finalized_view;
        bool found = false;
        for (const auto& existing_sym : data.symbol_pool) {
            if (existing_sym == sym_view) {
                finalized_view = existing_sym;
                found = true;
                break;
            }
        }
        if (!found) {
            data.symbol_pool.emplace_back(sym_view);
            finalized_view = data.symbol_pool.back();
        }

        data.ticks.push_back(Tick{timestamp, finalized_view, bid_px, ask_px, bid_qty, ask_qty});
    }

    return data;
}

void Engine::run_backtest(const std::vector<csot::Tick>& ticks, Strategy* strategy) {
    if (!strategy) {
        std::cerr << "Error: Cannot run backtest with a null strategy pointer.\n";
        return;
    }

    std::cout << "Calibrating CPU Time-Stamp Counter (TSC) frequency...\n";
    
    auto start_chrono = std::chrono::steady_clock::now();
    uint64_t start_tsc = __rdtsc();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    uint64_t end_tsc = __rdtsc();
    auto end_chrono = std::chrono::steady_clock::now();
    
    double elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_chrono - start_chrono).count();
    double elapsed_cycles = end_tsc - start_tsc;
    
    double ns_per_cycle = elapsed_ns / elapsed_cycles;
    std::cout << "Calibration complete. Factor: " << ns_per_cycle << " ns/cycle\n";

    std::cout << "Initializing strategy state...\n";
    strategy->on_init();

    std::cout << "Replaying " << ticks.size() << " market ticks along the assembly hot path...\n";
    LatencyHistogram hist;

    for (const auto& tick : ticks) {
        uint64_t c1 = __rdtsc();
        
        std::vector<Order> orders = strategy->on_tick(tick);
        
        uint64_t c2 = __rdtsc();

        benchmark::DoNotOptimize(orders);

        uint64_t delta_cycles = c2 - c1;
        auto ns = static_cast<uint64_t>(static_cast<double>(delta_cycles) * ns_per_cycle);
        
        hist.record(ns);
    }

    std::cout << "\n=== Backtest Complete Latency Summary (RDTSC-Measured) ===\n";
    hist.print(std::cout);
}

} // namespace csot