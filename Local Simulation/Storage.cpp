//
//  Storage.cpp
//  StockMarketSimulationLocal
//
//  Created by Thomas Ciha on 8/21/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#include "Storage.hpp"
#include "Order.h"
#include <string.h>
#include <iostream>

using namespace std;

void Account::deposit_cash(){
    cout << "How much cash would you like to deposit: " << endl;
    double c;
    cin >> c;
    balance += c;
}

pair<int, double> Account::get_short_pos(string tick){
    try{
        return ContainsShort[tick];
    }
    catch(...){
        return make_pair(0,0);
    }
}

pair<int, double> Account::get_long_pos(string tick){
    try{
        return ContainsStock[tick];
    }
    catch(...){
        return make_pair(0,0);
    }
}


void Account::update_contains_short(Order &o){
    bool already_own_some = false;                                                                      // assume that we don't own any stock
    long num_shares = 0, new_qty = 0;
    double avg_price = 0, new_avg_price = 0;
    
    for(auto & entry : ContainsShort){                                                                  // iterate over all short positions to see if we have a short position for the stock in this order
        if(entry.first == o.get_ticker()){                                                                    // if we have a short position
            num_shares = entry.second.first;                                                            // num of shares the account currently has shorted
            avg_price = entry.second.second;                                                            // avg price of those shares shorted
            already_own_some = true;                                                                    // in this case we 'own' a short position
        }
    }

    bool debug = false;
    if(debug){                                                                                          // display information for debugging purposes
        cout << "num_shares = " << num_shares << endl;
        cout << "avg price = " << avg_price << endl;
        cout << "already own some = " << already_own_some << endl;
        cout << "o.Size = " << o.get_size() << endl;
    }
    
    if(o.get_action() == Short){                                                                         // update qty and compute new_avg price
        new_qty = llabs(num_shares - o.get_size());                                                      // short positions are represented with negative numbers, thus subtracting here is actually increasing the position size
        new_avg_price = llabs((o.get_fill_price() * o.get_size()) + (avg_price * num_shares) / new_qty);
    }
    else{                                                                                               // if we bought to cover a short, i.e. o.OrderAction == Cover
        new_qty = num_shares + o.get_size();                                                            // compute update, addition here is reducing the size
        new_avg_price = avg_price;                                                                      // if we're covering, average price is unchanged
        if(already_own_some == false && debug)
            cout << "Error: cannot cover a position that doesn't exist" << endl;                        // for sanity purposes
    }
    
    if(new_qty != 0){
            ContainsShort[o.get_ticker()].first = new_qty;                                                // write new qty to table
            ContainsShort[o.get_ticker()].second = new_avg_price;                                         // write new avg price to table                                                                                   // update position
    }
    else
        ContainsShort.erase(o.get_ticker());                                                              // delete this entry from the table
}

void Account::add_order(Order &o){
    for(auto order : Orders){
        if(order.get_order_num() == o.get_order_num()){
            cout << "Error: Unable to add order to order history due to duplicate order number." << endl;
            return;
        }
    }
    Orders.push_back(o);
}

void Account::update_contains_stock(Order &o){
    bool already_own_some = false;                                                                      // assume that we don't own any stock
    long num_shares = 0, new_qty = 0;
    double avg_price = 0, new_avg_price = 0;
    
    for(auto &it : ContainsStock){                                                                      // check to see if we do own stock
        if(it.first == o.get_ticker()){
            num_shares = it.second.first;                                                               // get number of shares the account currently owns
            avg_price = it.second.second;                                                               // avg price of those shares owned
            already_own_some = true;
        }
    }
    
    if(o.get_action() == Buy){
        new_qty = o.get_size() + num_shares;                                                            // we initialized num_shares = 0, so if no position this still works
        new_avg_price = ((avg_price * num_shares) + (o.get_fill_price() * o.get_size())) / new_qty;
    }
    else{                                                                                               //sell order adjustment, say we own 10 shares w/ avg price of 50, we sell 5 shares for avg price of 40.
        new_qty = num_shares - o.get_size();
        new_avg_price = avg_price;
    }
    if(new_qty != 0){
        if(already_own_some){
            ContainsStock[o.get_ticker()].first = new_qty;                                              // update qty
            ContainsStock[o.get_ticker()].second = new_avg_price;                                       // update avg price
        }
        else {
            ContainsStock[o.get_ticker()].first = o.get_size();                                         // if we don't own some, use this order to update table
            ContainsStock[o.get_ticker()].second = o.get_fill_price();
        }
    }
    else {                                                                                              // If it's a sell order, remove position from contains stock
        ContainsStock.erase(o.get_ticker());                                                            // erase the order from the table
        if(o.get_action() != Sell)
            cout << "ERROR: removing stock position when order action != SELL";                         // for debugging / sanity checking purposes
    }
}

void Account::display_portfolio(){
    cout << "\nYour Assets:\nTicker\tQuantity PurchasePrice\n------  -------  ---------\n";
    for(unordered_map<string, pair<int, double>>::iterator it = ContainsStock.begin(); it != ContainsStock.end(); it++)            // Display Long Positions
        cout << it->first << "     " << it->second.first << "     " << it->second.second << endl;
    
    for(unordered_map<string, pair<int, double>>::iterator it = ContainsShort.begin(); it != ContainsShort.end(); it++)            // Display Short Positions
        cout << it->first << "     " << it->second.first << "     " << it->second.second << endl;
    
    cout << "\n" << endl;
}

void Account::display_account_info(){
    string status;
    (activity_status == 1) ? status = "Active" : status = "Frozen";
    cout<<"Your Account Information: \n\n";
    cout<< "AccountNo  Balance  ActivityStatus\n---------- ------- --------------\n";
    cout << "\t" << account_num << "\t\t" << balance << "\t" << status  << endl;
}
