#include "OrderBook.hpp"

int main() {
    OrderBook book;
    // Add multiple bids at different price levels
    book.add_order(1, 10050, 10, true);
    book.add_order(2, 10060, 5, true);   // Higher price, should be prioritized by std::greater
    book.add_order(3, 10040, 20, true);  // Lower price

    book.print_book();

    // Add a sell order that crosses the spread and matches against the best bid first (ID 2 at 10060)
    std::cout << "\n--- Incoming Sell Order ---\n";
    book.add_order(4, 10050, 7, false);

    book.print_book();
    return 0;
}
