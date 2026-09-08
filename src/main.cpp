#include "OrderBook.hpp"

int main() {
    OrderBook book;
    book.add_order(1, 10050, 10, true);   // Buy order
    book.add_order(2, 10045, 5, false);   // Sell order (crosses, leaves 5 remaining on ID 1)
    
    book.print_book();
    return 0;
}
