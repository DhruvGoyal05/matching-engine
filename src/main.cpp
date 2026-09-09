#include "OrderBook.hpp"
#include <iostream>

int main() {
    OrderBook book;

    // Bind an external trade listener
    book.set_trade_callback([](uint64_t buy_id, uint64_t sell_id, uint64_t price, uint32_t qty) {
        std::cout << "[CALLBACK] TRADE EXECUTED: " << qty << " units at " << price 
                  << " (Buy ID: " << buy_id << ", Sell ID: " << sell_id << ")\n";
    });

    book.add_order(1, 10050, 10, true);
    book.add_order(2, 10050, 4, false); // Triggers trade via callback

    book.print_book();
    return 0;
}
