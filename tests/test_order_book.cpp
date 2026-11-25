#include <cstdlib>
#include <iostream>

#include "order_book.h"

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            std::cerr << __FILE__ << ":" << __LINE__                       \
                      << " check failed: " << #condition << "\n";          \
            std::exit(1);                                                  \
        }                                                                  \
    } while (0)

namespace {
Trade trade(OrderId buy_id, OrderId sell_id, Price price, Quantity quantity) {
    return {buy_id, sell_id, price, quantity};
}

bool crossed(const OrderBook& book) {
    auto bid = book.best_bid();
    auto ask = book.best_ask();
    return bid && ask && *bid >= *ask;
}
void empty_book_has_no_best_prices() {
    OrderBook book;
    CHECK(!book.best_bid().has_value());
    CHECK(!book.best_ask().has_value());
    CHECK(book.size() == 0);
}

void resting_orders_set_best_prices() {
    OrderBook book;
    book.add_limit_order(1, Side::Buy, 100, 10);
    book.add_limit_order(2, Side::Buy, 99, 5);
    book.add_limit_order(3, Side::Sell, 101, 10);
    book.add_limit_order(4, Side::Sell, 102, 5);

    CHECK(book.best_bid() == 100);
    CHECK(book.best_ask() == 101);
    CHECK(book.size() == 4);
}

void cancelling_best_bid_promotes_next_level() {
    OrderBook book;
    book.add_limit_order(1, Side::Buy, 100, 10);
    book.add_limit_order(2, Side::Buy, 99, 5);

    CHECK(book.cancel(1));
    CHECK(book.best_bid() == 99);
    CHECK(book.size() == 1);
}

void cancelling_unknown_id_returns_false() {
    OrderBook book;
    book.add_limit_order(1, Side::Buy, 100, 10);

    CHECK(!book.cancel(42));
    CHECK(book.size() == 1);
}

void cancelling_last_order_at_a_price_removes_the_level() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 10);
    book.add_limit_order(2, Side::Sell, 101, 5);

    CHECK(book.cancel(1));
    CHECK(book.best_ask() == 101);
    CHECK(book.cancel(2));
    CHECK(!book.best_ask().has_value());
    CHECK(!book.cancel(2));
    CHECK(book.size() == 0);
}
void non_crossing_orders_do_not_trade() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 5);
    AddResult result = book.add_limit_order(2, Side::Buy, 100, 5);

    CHECK(result.accepted);
    CHECK(result.trades.empty());
    CHECK(book.size() == 2);
}

void full_fill_removes_the_resting_order() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 5);
    AddResult result = book.add_limit_order(2, Side::Buy, 101, 5);

    CHECK(result.trades.size() == 1);
    CHECK(result.trades[0] == trade(2, 1, 101, 5));
    CHECK(!book.best_ask().has_value());
    CHECK(!book.best_bid().has_value());
    CHECK(book.size() == 0);
}

void partial_fill_rests_the_remainder() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 4);
    AddResult result = book.add_limit_order(2, Side::Buy, 101, 10);

    CHECK(result.trades.size() == 1);
    CHECK(result.trades[0] == trade(2, 1, 101, 4));
    CHECK(!book.best_ask().has_value());
    CHECK(book.best_bid() == 101);
    CHECK(book.size() == 1);
}

void trade_happens_at_the_resting_price() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 5);
    AddResult result = book.add_limit_order(2, Side::Buy, 105, 5);

    CHECK(result.trades.size() == 1);
    CHECK(result.trades[0].price == 101);
}

void earlier_order_at_a_price_fills_first() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 5);
    book.add_limit_order(2, Side::Sell, 101, 5);
    AddResult result = book.add_limit_order(3, Side::Buy, 101, 5);

    CHECK(result.trades.size() == 1);
    CHECK(result.trades[0] == trade(3, 1, 101, 5));
    CHECK(!book.cancel(1));
    CHECK(book.cancel(2));
}

void partially_filled_order_keeps_its_place_in_the_queue() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 5);
    book.add_limit_order(2, Side::Sell, 101, 5);

    AddResult first = book.add_limit_order(3, Side::Buy, 101, 3);
    CHECK(first.trades.size() == 1);
    CHECK(first.trades[0] == trade(3, 1, 101, 3));

    AddResult second = book.add_limit_order(4, Side::Buy, 101, 4);
    CHECK(second.trades.size() == 2);
    CHECK(second.trades[0] == trade(4, 1, 101, 2));
    CHECK(second.trades[1] == trade(4, 2, 101, 2));
}

void buy_walks_up_the_asks() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 5);
    book.add_limit_order(2, Side::Sell, 101, 3);
    book.add_limit_order(3, Side::Sell, 102, 4);

    AddResult result = book.add_limit_order(4, Side::Buy, 102, 10);
    CHECK(result.trades.size() == 3);
    CHECK(result.trades[0] == trade(4, 1, 101, 5));
    CHECK(result.trades[1] == trade(4, 2, 101, 3));
    CHECK(result.trades[2] == trade(4, 3, 102, 2));
    CHECK(book.best_ask() == 102);
    CHECK(!book.best_bid().has_value());
    CHECK(book.size() == 1);

    // Order 3 had 2 left, so a buy of 2 clears the book.
    AddResult rest = book.add_limit_order(5, Side::Buy, 102, 2);
    CHECK(rest.trades.size() == 1);
    CHECK(rest.trades[0] == trade(5, 3, 102, 2));
    CHECK(book.size() == 0);
}

void sell_walks_down_the_bids() {
    OrderBook book;
    book.add_limit_order(1, Side::Buy, 100, 5);
    book.add_limit_order(2, Side::Buy, 100, 3);
    book.add_limit_order(3, Side::Buy, 99, 4);

    AddResult result = book.add_limit_order(4, Side::Sell, 99, 10);
    CHECK(result.trades.size() == 3);
    CHECK(result.trades[0] == trade(1, 4, 100, 5));
    CHECK(result.trades[1] == trade(2, 4, 100, 3));
    CHECK(result.trades[2] == trade(3, 4, 99, 2));
    CHECK(book.best_bid() == 99);
    CHECK(!book.best_ask().has_value());
    CHECK(book.size() == 1);
}

void limit_price_stops_the_walk() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 5);
    book.add_limit_order(2, Side::Sell, 103, 5);

    AddResult result = book.add_limit_order(3, Side::Buy, 102, 10);
    CHECK(result.trades.size() == 1);
    CHECK(result.trades[0] == trade(3, 1, 101, 5));
    CHECK(book.best_bid() == 102);
    CHECK(book.best_ask() == 103);
    CHECK(!crossed(book));
}
void market_buy_sweeps_several_levels() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 4);
    book.add_limit_order(2, Side::Sell, 102, 4);

    AddResult result = book.add_market_order(3, Side::Buy, 6);
    CHECK(result.accepted);
    CHECK(result.trades.size() == 2);
    CHECK(result.trades[0] == trade(3, 1, 101, 4));
    CHECK(result.trades[1] == trade(3, 2, 102, 2));
    CHECK(book.best_ask() == 102);
    CHECK(book.size() == 1);
}

void market_sell_sweeps_several_levels() {
    OrderBook book;
    book.add_limit_order(1, Side::Buy, 100, 4);
    book.add_limit_order(2, Side::Buy, 99, 4);

    AddResult result = book.add_market_order(3, Side::Sell, 6);
    CHECK(result.trades.size() == 2);
    CHECK(result.trades[0] == trade(1, 3, 100, 4));
    CHECK(result.trades[1] == trade(2, 3, 99, 2));
    CHECK(book.best_bid() == 99);
}

void market_order_remainder_is_discarded() {
    OrderBook book;
    book.add_limit_order(1, Side::Sell, 101, 6);

    AddResult result = book.add_market_order(2, Side::Buy, 10);
    CHECK(result.trades.size() == 1);
    CHECK(result.trades[0] == trade(2, 1, 101, 6));
    CHECK(!book.best_bid().has_value());
    CHECK(!book.best_ask().has_value());
    CHECK(book.size() == 0);
}

void market_order_into_an_empty_book_does_nothing() {
    OrderBook book;
    AddResult result = book.add_market_order(1, Side::Buy, 5);

    CHECK(result.accepted);
    CHECK(result.trades.empty());
    CHECK(book.size() == 0);
}
}  // namespace

int main() {
    empty_book_has_no_best_prices();
    resting_orders_set_best_prices();
    cancelling_best_bid_promotes_next_level();
    cancelling_unknown_id_returns_false();
    cancelling_last_order_at_a_price_removes_the_level();
        non_crossing_orders_do_not_trade();
    full_fill_removes_the_resting_order();
    partial_fill_rests_the_remainder();
    trade_happens_at_the_resting_price();
    earlier_order_at_a_price_fills_first();
    partially_filled_order_keeps_its_place_in_the_queue();
    buy_walks_up_the_asks();
    sell_walks_down_the_bids();
    limit_price_stops_the_walk();
        market_buy_sweeps_several_levels();
    market_sell_sweeps_several_levels();
    market_order_remainder_is_discarded();
    market_order_into_an_empty_book_does_nothing();
    std::cout << "all tests passed\n";
}