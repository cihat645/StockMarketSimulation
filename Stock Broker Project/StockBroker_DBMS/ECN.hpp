//
//  ECN.hpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 2/16/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#ifndef ECN_hpp
#define ECN_hpp

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "Offer.hpp"
#include "Order.h"

// I needed to establish a communication framework between the OMS and the ECN to update the necessary info when an order has been processed. The order_info strcuture is the rudimentary protocol I've developed to accomplish this.

struct order_info{
    float avg_price;
    int shares_filled;
    Status offer_status;
    int order_ID; // maps order_info to a particular order.
    bool incomplete_market_order = false; // used as a flag when a market order is partially filled at one ECN as a result of inadequate liquidity. The OMS will route the market order to another ECN to fulfill the rest of the order if this value is true.

    order_info(float f, int q, Status s, int oID, bool flag){
        avg_price = f;
        shares_filled = q;
        offer_status = s;
        order_ID = oID;
        incomplete_market_order = flag;
    }

    void Print(){
        cout << "ORDER ID: " << order_ID << endl;
        cout << "Avg Price: " << avg_price << endl;
        cout << "Shares Filled: " << shares_filled << endl;
        cout << "Order Status: ";
        switch(offer_status){
            case 0: cout << "Filled" << endl;
                break;
            case 1: cout << "Pending" << endl;
                break;
            case 2: cout << "Canceled" << endl;
                break;
            case 3: cout << "Aborted" << endl;
                break;

        }
        (incomplete_market_order) ? cout << "incomplete_market_order : true" << endl : cout << "incomplete_market_order: false\n " << endl;
    }
};

class Market{
    private:
    string Ticker;
    vector<Offer> current_bids, current_asks;

    
    public:
    void Print_Offers(Type t);
    void insert_front(Offer &o);
    void remove_best_ask();
    void remove_best_bid();

    void NEW_insert_offer(Offer &o);

    float get_best_ask_price(){ return (current_asks.empty()) ? 0.0 : current_asks[0].getPrice();} // return 0.0 if empty
    Offer get_best_ask(){ return (current_asks.empty()) ? Offer() :current_asks[0];}//if no asks, return NULL
    float get_best_bid_price(){return (current_bids.empty()) ? 0.0 :current_bids[0].getPrice() ; }// return a price of 0.0 if empty
    Offer get_best_bid(){ return (!current_bids.empty()) ? current_bids[0] : Offer(); } //if no asks, return NULL
    

    Market(string s){
        Ticker = s;
    }
};


class ECN{
private:
    string ECN_Name;
    long num_of_transactions;
    class Market market; // we will have to create a data structure to hold many market objects if we want to create an ECN which trades many stocks.
    vector<order_info> ParseOffer(Offer &offer); // this function will parse offers, basically decides if the offer to be added can be used to form a transaction given the current bids and asks or if the offer should be inserted.
    
    vector<order_info> make_transaction_ask_NEW(Offer &o);
    vector<order_info> make_transaction_bid_NEW(Offer &o);
    
public:
    ECN(string s) : market("IBM") { // right now we're assuming each ECN has only one stock, IBM
        ECN_Name = s;
        num_of_transactions = 0;
    }

    //void market_close(); // this function will submit all order of type MARKET ON CLOSE and call clear_day_orders(). Basically will do anything that needs to be done at market close.
    //void clear_day_orders(); // will clear all day orders from current_bids / current_asks that were placed within the application. Will change their status to aborted.
};


#endif /* ECN_hpp */
