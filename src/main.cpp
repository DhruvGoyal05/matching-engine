#include "OrderBook.hpp"

int main() {
    OrderBook book;
    book.add_order(1, 10050, 10, true);   // Add limit buy order
    book.print_book();
    
    book.cancel_order(1);                 // Cancel it
    book.print_book();                    // Verify book is empty
    return 0;
}
