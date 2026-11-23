#include "order_book.h"

#include <iterator>

void OrderBook::add_limit_order(OrderId id, Side side, Price price, Quantity quantity) {
    Level& level = (side == Side::Buy) ? bids_[price] : asks_[price];
    level.push_back({id, side, price, quantity});
    index_.emplace(id, Location{side, price, std::prev(level.end())});
}

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

std::size_t OrderBook::size() const {
    return index_.size();
}