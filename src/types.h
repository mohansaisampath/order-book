#pragma once

#include <cstdint>
#include <vector>

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint32_t;

enum class Side { Buy, Sell };

struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity quantity;
};

struct Trade {
    OrderId buy_id;
    OrderId sell_id;
    Price price;
    Quantity quantity;
};

inline bool operator==(const Trade& a, const Trade& b) {
    return a.buy_id == b.buy_id && a.sell_id == b.sell_id && a.price == b.price &&
           a.quantity == b.quantity;
}

struct AddResult {
    bool accepted;
    std::vector<Trade> trades;
};