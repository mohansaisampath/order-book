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

}  // namespace

int main() {
    empty_book_has_no_best_prices();
    resting_orders_set_best_prices();
    cancelling_best_bid_promotes_next_level();
    cancelling_unknown_id_returns_false();
    cancelling_last_order_at_a_price_removes_the_level();
    std::cout << "all tests passed\n";
}