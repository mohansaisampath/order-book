#include "order_book.h"

#include <algorithm>
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

// Would an incoming order with this limit trade at the best opposite price?
bool crosses(Side incoming, Price limit, Price best) {
    return incoming == Side::Buy ? best <= limit : best >= limit;
}

}  // namespace

template <typename Book>
Quantity OrderBook::match_against(Book& opposite, OrderId id, Side side, Price limit,
                                  Quantity quantity, std::vector<Trade>& trades) {
    while (quantity > 0 && !opposite.empty()) {
        auto level = opposite.begin();
        if (!crosses(side, limit, level->first)) {
            break;
        }

        Level& orders = level->second;
        while (quantity > 0 && !orders.empty()) {
            Order& resting = orders.front();
            Quantity filled = std::min(quantity, resting.quantity);

            OrderId buy_id = (side == Side::Buy) ? id : resting.id;
            OrderId sell_id = (side == Side::Buy) ? resting.id : id;
            trades.push_back({buy_id, sell_id, level->first, filled});

            quantity -= filled;
            resting.quantity -= filled;
            if (resting.quantity == 0) {
                index_.erase(resting.id);
                orders.pop_front();
            }
        }

        if (orders.empty()) {
            opposite.erase(level);
        }
    }
    return quantity;
}

Quantity OrderBook::match_order(OrderId id, Side side, Price limit, Quantity quantity,
                                std::vector<Trade>& trades) {
    if (side == Side::Buy) {
        return match_against(asks_, id, side, limit, quantity, trades);
    }
    return match_against(bids_, id, side, limit, quantity, trades);
}

void OrderBook::rest_order(OrderId id, Side side, Price price, Quantity quantity) {
    Level& level = (side == Side::Buy) ? bids_[price] : asks_[price];
    level.push_back({id, side, price, quantity});
    index_.emplace(id, Location{side, price, std::prev(level.end())});
}

AddResult OrderBook::add_limit_order(OrderId id, Side side, Price price, Quantity quantity) {
    AddResult result{true, {}};
    Quantity left = match_order(id, side, price, quantity, result.trades);
    if (left > 0) {
        rest_order(id, side, price, left);
    }
    return result;
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