#pragma once

#include "histogram.hpp"
#include "strategy.hpp"
#include <deque>
#include <string>
#include <vector>

namespace csot {

struct LoadedMarketData {
    std::deque<std::string> symbol_pool;
    std::vector<csot::Tick> ticks;
};

class Engine {
  public:
    Engine() = default;
    ~Engine() = default;

    LoadedMarketData load_ticks(const std::string &path);

    void run_backtest(const std::vector<csot::Tick> &ticks, Strategy *strategy);
};

} // namespace csot