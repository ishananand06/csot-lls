#include "strategy.hpp"

namespace csot {

class NullStrategy : public Strategy {
  public:
    NullStrategy() = default;
    virtual ~NullStrategy() = default;

    void on_init() override {}

    std::vector<Order> on_tick(const Tick &t) override {
        (void)t;
        return {};
    }
};

} // namespace csot

extern "C" csot::Strategy *create_strategy() {
    return new csot::NullStrategy();
}