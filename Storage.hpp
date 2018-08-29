//
//  Storage.hpp
//  StockMarketSimulationLocal
//
//  Created by Thomas Ciha on 8/21/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#ifndef Storage_hpp
#define Storage_hpp

#include <stdio.h>
#include <unordered_map>
#include <string.h>
#include <vector>
#include "Order.h"

using namespace std;

class Account{
private:
    double account_num, balance;
    int activity_status;                                        // 1 means active, 0 means inactive
    unordered_map<string, pair<int, double>> ContainsStock;     // Ticker -> (Qty, purchase price)
    unordered_map<string, pair<int, double>> ContainsShort;     // Ticker -> (Qty, short price)
    vector<Order> Orders;                                       // Order information
    
public:
    
    Account(double acc_num){
        account_num = acc_num;
        activity_status = 1;                                    // account start off being active
        balance = 0;                                            // balance starts at 0
    }
    
    double get_account_num(){return account_num; }
    double get_balance() {return balance;}
    int get_activity_status(){return activity_status;}
    pair<int, double> get_short_pos(string tick);
    pair<int, double> get_long_pos(string tick);
    int get_num_orders() {return Orders.size(); }
    
    void add_order(Order &o);
    void set_balance(double b) {balance = b;}
    void deposit_cash();
    void display_orders() {if(Orders.size() > 0) for(auto o : Orders) o.Print(); else cout << "No orders to display" << endl;}
    void display_portfolio();
    void display_account_info();
    void update_contains_stock(Order &o);
    void update_contains_short(Order &o);
    void add_long_position(string tick, int qty, double price_paid) {ContainsStock[tick] = make_pair(qty, price_paid); }
    void add_short_position(string tick, int qty, double price_shorted) {ContainsShort[tick] = make_pair(-1 * qty, price_shorted); }

};








#endif /* Storage_hpp */
