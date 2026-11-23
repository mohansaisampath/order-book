#include "order_book.h"

#include <iterator>

namespace {

template <typename Book>
void erase_order(Book& book, Price price, std::list<Order>::iterator position) {
    auto level = book.find(price);
    level->second.erase(position);
    if (level->second.empty()) {
        book.erase(level);
    }
}

}  // namespace

void OrderBook::add_limit_order(OrderId id, Side side, Price price, Quantity quantity) {
    Level& level = (side == Side::Buy) ? bids_[price] : asks_[price];
    level.push_back({id, side, price, quantity});
    index_.emplace(id, Location{side, price, std::prev(level.end())});
}

bool OrderBook::cancel(OrderId id) {
    auto found = index_.find(id);
    if (found == index_.end()) {
        return false;
    }

    const Location& where = found->second;
    if (where.side == Side::Buy) {
        erase_order(bids_, where.price, where.position);
    } else {
        erase_order(asks_, where.price, where.position);
    }
    index_.erase(found);
    return true;
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