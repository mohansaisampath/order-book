#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>

#include "types.h"

class OrderBook {
public:
    void add_limit_order(OrderId id, Side side, Price price, Quantity quantity);

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    std::size_t size() const;

private:
    // A list keeps arrival order, and erasing one order leaves the iterators
    // of all the others valid. Cancel depends on both.
    using Level = std::list<Order>;

    struct Location {
        Side side;
        Price price;
        Level::iterator position;
    };

    std::map<Price, Level, std::greater<Price>> bids_;
    std::map<Price, Level> asks_;
    std::unordered_map<OrderId, Location> index_;
};