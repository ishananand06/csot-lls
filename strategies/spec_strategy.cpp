#include "strategy.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

namespace csot {

struct SymbolState {
    double mids[64]{};
    uint32_t count = 0;
    uint32_t head = 0;
    int32_t position = 0;

    double rolling_sum = 0.0;
    double rolling_sq_sum = 0.0;
};

class SpecStrategy : public Strategy {
  private:
    std::vector<Order> orders_receptacle_;
    std::array<std::string_view, 64> active_symbols_;
    std::array<SymbolState, 64> symbol_states_;
    uint32_t unique_symbol_count_ = 0;

    inline SymbolState &get_or_create_state(std::string_view symbol) __attribute__((always_inline)) {
        const char* target_ptr = symbol.data();
        const size_t target_len = symbol.size();

        #pragma GCC unroll 4
        for (uint32_t i = 0; i < unique_symbol_count_; ++i) {
            if (active_symbols_[i].data() == target_ptr && active_symbols_[i].size() == target_len) {
                return symbol_states_[i];
            }
        }

        if (unique_symbol_count_ < 64) {
            active_symbols_[unique_symbol_count_] = symbol;
            SymbolState& state = symbol_states_[unique_symbol_count_];
            unique_symbol_count_++;
            return state;
        }
        return symbol_states_[63];
    }

  public:
    SpecStrategy() = default;
    virtual ~SpecStrategy() = default;

    void on_init() override {
        orders_receptacle_.reserve(1);
    }

    std::vector<Order> on_tick(const Tick &t) override {
        orders_receptacle_.clear();

        const double current_bid = t.bid_px;
        const double current_ask = t.ask_px;
        const double mid_price = (current_bid + current_ask) * 0.5;

        SymbolState &state = get_or_create_state(t.symbol);
        const double old_price = state.mids[state.head];
        state.mids[state.head] = mid_price;
        state.head = (state.head + 1) & 63;

        if (state.count < 64) {
            state.rolling_sum += mid_price;
            state.rolling_sq_sum += mid_price * mid_price;
            ++state.count;
            if (state.count < 64) {
                return orders_receptacle_;
            }
        } else {
            state.rolling_sum += (mid_price - old_price);
            state.rolling_sq_sum += (mid_price * mid_price) - (old_price * old_price);
        }

        const double mean = state.rolling_sum / 64.0;
        double variance = (state.rolling_sq_sum / 64.0) - (mean * mean);
        if (variance < 0.0) variance = 0.0; 

        if (variance < 1e-18) {
            return orders_receptacle_;
        }

        const double diff = mid_price - mean;
        const double diff_sq = diff * diff;

        const int32_t current_pos = state.position;
        const bool is_zero_pos = (current_pos == 0);

        const bool entry_triggered = (diff_sq >= 4.0 * variance);
        const bool is_diff_positive = (diff > 0.0);
        const Order::Side entry_side = is_diff_positive ? Order::Side::SELL : Order::Side::BUY;
        const double entry_price = is_diff_positive ? current_bid : current_ask;

        const bool exit_triggered = (diff_sq <= 0.25 * variance);
        const bool is_pos_positive = (current_pos > 0);
        const Order::Side exit_side = is_pos_positive ? Order::Side::SELL : Order::Side::BUY;
        const double exit_price = is_pos_positive ? current_bid : current_ask;
        const uint32_t exit_qty = is_pos_positive ? static_cast<uint32_t>(current_pos) : static_cast<uint32_t>(-current_pos);

        const bool trigger_order = is_zero_pos ? entry_triggered : exit_triggered;
        const Order::Side pending_side = is_zero_pos ? entry_side : exit_side;
        const double pending_price = is_zero_pos ? entry_price : exit_price;
        const uint32_t pending_qty = is_zero_pos ? 1 : exit_qty;

        if (trigger_order) {
            orders_receptacle_.push_back(Order{pending_side, t.symbol, pending_price, pending_qty});
        }

        return orders_receptacle_;
    }

    void on_fill(const Order &o, double fill_price, uint32_t fill_qty) override {
        (void)fill_price;
        SymbolState &state = get_or_create_state(o.symbol);
        if (o.side == Order::Side::BUY) {
            state.position += static_cast<int32_t>(fill_qty);
        } else {
            state.position -= static_cast<int32_t>(fill_qty);
        }
    }
};

} // namespace csot

extern "C" csot::Strategy *create_strategy() {
    return new csot::SpecStrategy();
}