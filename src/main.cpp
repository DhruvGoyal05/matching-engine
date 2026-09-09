#include "OrderBook.hpp"
#include <iostream>
#include <string>
#include <sstream>

// Simple FIX Message Parser helper (Extracts tag values)
std::string get_fix_tag_value(const std::string& fix_msg, int tag) {
    std::string search_str = std::to_string(tag) + "=";
    size_t start = fix_msg.find(search_str);
    if (start == std::string::npos) return "";
    start += search_str.length();
    size_t end = fix_msg.find('|', start);
    if (end == std::string::npos) end = fix_msg.length();
    return fix_msg.substr(start, end - start);
}

int main() {
    OrderBook book;
    book.set_trade_callback([](uint64_t buy_id, uint64_t sell_id, uint64_t price, uint32_t qty) {
        std::cout << "[TRADE] Executed " << qty << " units at price " << price 
                  << " (Buy ID: " << buy_id << ", Sell ID: " << sell_id << ")\n";
    });

    // 1. Setup initial limit order on the book
    book.add_order(1, 10050, 10, true, OrderType::LIMIT);

    // 2. Simulate incoming FIX Message for an IOC Order
    // Tag 35=D (New Order), Tag 54=1 (Buy) or 2 (Sell), Tag 38=Qty, Tag 44=Price, Tag 40=OrderType (3=IOC)
    std::string fix_order_ioc = "8=FIX.4.2|35=D|11=102|54=2|38=15|44=10050|40=3|";
    
    std::cout << "\nParsing FIX Message (IOC):\n" << fix_order_ioc << "\n";
    uint64_t id = std::stoull(get_fix_tag_value(fix_order_ioc, 11));
    bool is_buy = (get_fix_tag_value(fix_order_ioc, 54) == "1");
    uint32_t qty = std::stoul(get_fix_tag_value(fix_order_ioc, 38));
    uint64_t price = std::stoull(get_fix_tag_value(fix_order_ioc, 44));
    
    // Inject parsed IOC order
    book.add_order(id, price, qty, is_buy, OrderType::IOC);

    book.print_book();
    return 0;
}
