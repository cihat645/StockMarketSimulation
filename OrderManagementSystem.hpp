//
//  OrderManagementSystem.hpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 8/16/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#ifndef OrderManagementSystem_hpp
#define OrderManagementSystem_hpp

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include "Order.h"
#include "ECN.hpp"
#include "Storage.hpp"

using namespace std;

// ========================================================================== SOURCES ====================================================================================================================================
// EMS (Execution Management Systems)http://www.wallstreetandtech.com/trading-technology/execution-management-systems-from-the-street-and-on-the-block-/d/d-id/1258066?
//      - An EMS is a software application that provides market access to a user with real time data, numerous algorithms and analytical tools.

// TCA - Transaction Cost Analysis - is a tool utilized by instutitional investors to analyze the cost of a trade route to determine the optimal route for execution.

// source: https://en.wikipedia.org/wiki/Transaction_cost_analysis

// Here is some additional info on how brokers process orders: https://www.sec.gov/reportspubs/investor-publications/investorpubstradexechtm.html
// ========================================================================================================================================================================================================================
// What does an OMS do?
//   - The order management system (OMS) places orders in a queue for processing, rebalances portfolios, documents orders and transaction history.

//It needs to take into consideration not only the price and quantity of shares, but also the type of order. It also needs to take into consideration the possibility and likelyhood of price improvement.

// It takes an order from one of the broker's clients as input and then makes a decision as to whether the order should be internalized, sent to an exchange or sent to an ECN.
// ========================================================================================================================================================================================================================


class OMS{                                                                                                                  // Acts as the OMS and stock broker, in reality, OMS won't handle storage for accounts
private:
    int commission_rate = 10;                                                                                               // charge per trade
    float internalization_profit = 0;                                                                                       // profit resulting from internalization

    vector<Account *> Accounts;                                                                                             // Accounts registered with the OMS
    vector<Order> pending_orders;                                                                                           // tracks every order created by us until it has been filled by an ECN
    vector<class ECN *> available_ECNs;                                                                                     // Available ECNs, ECNS that the OMS is connected to
    unordered_map<string, pair<int, double>> StockInventory;                                                                // The broker's stock inventory: Ticker -> (Qty, Purhcase price)
    
    pair<bool, string> eligible_order(Order &o);                                                                            // determines whether or not an order is eligible based on the user's account information
    pair<float, int> get_best_mkt_price(Type &o);                                                                           // Type is the type for OfferType
    order_info internalize_order(Offer &o, pair<int,double> internal_inventory, float i_price, string tick);
    bool internalization_profitable(Order const &o, float const i_price, const float m_price);                              // returns true if we can profit off internalization
    pair<int, double> get_internalization_price_and_qty(string tick);                                                         // obtains the price the stock broker paid for the stock with at ticker of 'tick'
    vector<Order>::iterator find_pending_order(int id);                                                                     // returns an iterator pointing to the order which corresponds to the order id parameter.

public:
    vector<order_info> ProcessOrder(Order &o);                                                                              // the function that takes a new order submitted by a client and prepares it for processing
    void update_orders(vector<order_info> &vec);                                                                            // See #1 below.
    void UpdateAccount(Order &o);                                                                                           // Once an order has been fulfilled, use info to update account
    void DisplayPendingOrders();                                                                                            // prints all orders that are waiting to be filled
    void AddAccount(Account *a);                                                                                            // Add an account to the OMS
    void DisplayAccounts();
    void AddStock(string tick, int qty, double price){StockInventory[tick] = make_pair(qty, price);}                        // used to add stock inventory to broker
    void DisplayInventory();
    
    // Unfortunately, I didn't have the chance to test all of the queries for the internalization process. However, the logic and rough draft queries are included within these functions to illustrate the method undertaken by the OMS to make order routing decisions. If you're reading this, feel free to pick it up and implement it.
    

    OMS(vector<class ECN *> ecns){
        for(vector<class ECN *>::iterator it = ecns.begin(); it != ecns.end(); it++)
            available_ECNs.push_back(*it);
    }

    float get_internalization_profit(){
        return internalization_profit;
    }
    
    //queue<Order> end_of_day;                                                                                                  // this queue holds all orders that are meant to be executed upon market close
    //queue<Order> begin_of_day;                                                                                                // this queue holds all orders to be submitted upon market open
};

/*
 Function Descriptions:
 
 1. update_orders - this function makes the appropritate changes to orders in the pending orders after the OMS has received relevant order_info pertaining to the filling of that order. A concrete example will help elucidate the purpose of this function. Say an ECN was able to fill 500 of the 1000 shares for a buy limit order due to the entered price and current market conditions (current market price, volume, etc.). The ECN will then add the limit order to current_bids and it will stay there until filled or the term of the order is violated (e.g. end of day for an order with an order term = Day). Right after adding the limit order to current_bids, the ECN which partially filled this order sends an order_info object to the OMS which includes the number of shares that were filled (500) and the average price obtained for those shares. The update_order function of the OMS then uses this information to adjust the average fill price and number of shares filled for the order in the pending_orders vector. Once the rest of the order has been filled (the order_info object would then have a status of 'Filled'), the order is removed from the pending_orders and added to the DBMS.
 
 
 
 */

#endif /* OrderManagementSystem_hpp */
