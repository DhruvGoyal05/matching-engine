#include "OrderBook.hpp"

int main() {
    OrderBook book;
    book.add_order(1, 10050, 10, true);     // Limit Buy order
    book.add_order(2, 0, 4, false, true);   // Market Sell order (executes immediately against ID 1)
    
    book.print_book();
    return 0;
}
