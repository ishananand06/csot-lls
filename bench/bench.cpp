#include "../strategies/spec_strategy.cpp"
#include "strategy.hpp"
#include <benchmark/benchmark.h>

static void BM_SpecStrategy_OnTick_Warm(benchmark::State &state) {
    csot::SpecStrategy strategy;
    strategy.on_init();

    csot::Tick tick;
    tick.timestamp_ns = 1700000000000000000ULL;
    tick.symbol = "SYM0";
    tick.bid_px = 100.00;
    tick.ask_px = 100.02;
    tick.bid_qty = 500;
    tick.ask_qty = 500;

    for (int i = 0; i < 70; ++i) {
        tick.bid_px += 0.01;
        tick.ask_px += 0.01;
        strategy.on_tick(tick);
    }

    for (auto _ : state) {
        tick.bid_px = (tick.bid_px == 100.00) ? 100.05 : 100.00;
        tick.ask_px = tick.bid_px + 0.02;
        auto orders = strategy.on_tick(tick);
        benchmark::DoNotOptimize(orders);
    }
}

BENCHMARK(BM_SpecStrategy_OnTick_Warm);

BENCHMARK_MAIN();