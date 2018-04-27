//
//  OrderManagementSystem.hpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 2/16/18.
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
#include "C:\Users\thomasciha\Oracle\SQLAPI\include\SQLAPI.h"

using namespace std;

// ==================== SOURCES ========================
// EMS (Execution Management Systems)http://www.wallstreetandtech.com/trading-technology/execution-management-systems-from-the-street-and-on-the-block-/d/d-id/1258066?
//      - An EMS is a software application that provides market access to a user with real time data, numerous algorithms and analytical tools.

// TCA - Transaction Cost Analysis - is a tool utilized by instutitional investors to analyze the cost of a trade route to determine the optimal route for execution.

// source: https://en.wikipedia.org/wiki/Transaction_cost_analysis

// Here is some additional info on how brokers process orders: https://www.sec.gov/reportspubs/investor-publications/investorpubstradexechtm.html
// ======================================================

// What does an OMS do?
//   - The order management system (OMS) places orders in a queue for processing, rebalances portfolios, documents orders and transaction history.

//It needs to take into consideration not only the price and quantity of shares, but also the type of order. It also needs to take into consideration the possibility and likelyhood of price improvement.

// It takes an order from one of the broker's clients as input and then makes a decision as to whether the order should be internalized, sent to an exchange or sent to an ECN.

// ======================================================
struct stock { // stock inventory structure used by OMS, this is used to store data from StockInventory when analyzing the opportunity to internalize
    string ticker;
    float price;
    int qty;
    stock(string t, float p, int q){
        ticker = t;
        price = p;
        qty = q;
    }
};

class OMS{
    private:
    int commission_rate = 10;           // charge per trade

    /*Unfortunately, we did not have enough time to implement market on open or market on close orders. However, we would have utilized the following queues to handle these types of orders and cleared the queues once the appropriate timestamp had been parsed from the market data.
    queue<Order> end_of_day; // this queue holds all orders that are meant to be executed upon market close
    queue<Order> begin_of_day;  // this queue holds all orders to be submitted upon market open   */
    
    vector<class ECN *> available_ECNs; // Available ECNs, ECNS that the OMS is connected to
    
    pair<bool, string> eligible_order(Order &o, SAConnection &con, long AccNum); // determines whether or not an order is eligible based on the user's account information
    pair<float, int> get_best_mkt_price(Type &o); // Type is the type for OfferType
    
    vector<Order> pending_orders; // this will keep track of every order that is created until it has been filled by an ECN. We add orders to this to be processed at a timestamp of XYZ and then subimt them when iterating through the time series data
    
    void update_orders(vector<order_info> &vec, SAConnection &con, long AccNum); // see #1 below.
    vector<Order>::iterator find_pending_order(int id); // returns an iterator pointing to the order which corresponds to the order id parameter.
    void Update_OrderTable(Order &o, SAConnection &con, long AccNum); // this function updates the DBMS for orders that have been filled
    
    order_info internalize_order(Offer &o, pair<float,int> internal_inventory, float i_price, string tick, SAConnection &con, long AccNum);
    bool internalization_profitable(Order const &o, float const i_price, const float m_price); // returns true if we can profit off internalization
    pair<float,int> get_internalization_price_and_qty(string tick, SAConnection &con, long AccNum); // obtains the price the stock broker paid for the stock with at ticker of 'tick'
    

    
    public:
    void DisplayPendingOrders(); // prints all orders that are waiting to be filled
    vector<order_info> ProcessOrder(Order &o, SAConnection &con, long AccNum);


    OMS(vector<class ECN *> ecns){
        for(vector<class ECN *>::iterator it = ecns.begin(); it != ecns.end(); it++)
            available_ECNs.push_back(*it);
    }

};

/*
 Function Descriptions:
 
 1. update_orders - this function makes the appropritate changes to orders in the pending orders after the OMS has received relevant order_info pertaining to the filling of that order. A concrete example will help elucidate the purpose of this function. Say an ECN was able to fill 500 of the 1000 shares for a buy limit order due to the entered price and current market conditions (current market price, volume, etc.). The ECN will then add the limit order to current_bids and it will stay there until filled or the term of the order is violated (e.g. end of day for an order with an order term = Day). Right after adding the limit order to current_bids, the ECN which partially filled this order sends an order_info object to the OMS which includes the number of shares that were filled (500) and the average price obtained for those shares. The update_order function of the OMS then uses this information to adjust the average fill price and number of shares filled for the order in the pending_orders vector. Once the rest of the order has been filled (the order_info object would then have a status of 'Filled'), the order is removed from the pending_orders and added to the DBMS.
 
 */



#endif /* OrderManagementSystem_hpp */
