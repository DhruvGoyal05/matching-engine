#include "OrderBook.hpp"

int main() {
    OrderBook book;
    book.add_order(1, 10050, 10, true);   // Buy order
    book.add_order(2, 10055, 5, false);  // Sell order
    return 0;
}
