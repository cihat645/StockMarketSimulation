//
//  OrderManagementSystem.cpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 8/16/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#include "OrderManagementSystem.hpp"
#include <iostream>
#include <limits>
#include <stdio.h>
#include "Order.h"
#include "Offer.hpp"
#include "ECN.hpp"
#include <utility>

using namespace std;

// ================================================== ABOUT THE OMS ======================================================================

// The OMS must compute the benefits of internalizing the order (if feasible) vs. sending it to an ECN or market maker. This means the OMS needs to contemplate the trade-off between capital gain via internalization vs discovering optimal ECN routing method. It must also do so in compliance with SEC regulations. Market makers (and OTC market makers) incentivize brokers to route orders to them by offering a form of payment, called "Payment for order flow". We will not be incorporating this into the project as of now.

// As stated on the SEC website: In deciding how to execute orders, your broker has a duty to seek the best execution that is reasonably available for its customers' orders. That means your broker must evaluate the orders it receives from all customers in the aggregate and periodically assess which competing markets, market makers, or ECNs offer the most favorable terms of execution.

// When a broker scans the market in search of better execution prices, they are effectively looking for the "opportunity of price improvement". "Price improvement" is the opportunity, but not the guarantee, for an order to be executed at a better price than the current quote. (I'm assuming the current quote is the price of the latest trade).

// This function will process the front-most order in the queue, which consists of converting the order in to an offer and then sending that to either the ECN or internalizing the offer.

/*  Three potential choices for routing:
    1) ECN
    2) Broker's Inventory (internalize order)
    3) Match order with order from another client
 */

// ========================================================================================================================================

void OMS::AddAccount(Account *a){
    for(vector<Account *>::iterator acc = Accounts.begin(); acc != Accounts.end(); acc++){
        double acc_num = (*acc)->get_account_num();
        if(acc_num == a->get_account_num()){
            cout << "Error: cannot add accounts with duplicate account numbers" << endl;
            return;
        }
    }
    Accounts.push_back(a);                                                                      // if no other account has this account number, add it to the list
}

void OMS::DisplayAccounts(){
    for(vector<Account *>::iterator it = Accounts.begin(); it != Accounts.end(); it++)
        (*it)->display_account_info();
}

void OMS::DisplayPendingOrders(){
    if(!pending_orders.empty()){
        cout << "Displaying pending_orders: " << endl;
        for(int i = 0; i < pending_orders.size(); i++){
            pending_orders[i].Print();
        }
    }
    else
        cout << "No Pending Orders Exist" << endl;
}

void OMS::DisplayInventory(){
    cout << "\n---- OMS INVENTORY -----" << endl;
    cout << "Ticker\tQty\tPurhcasePrice" << endl;
    for(unordered_map<string, pair<int, double>>::iterator it = StockInventory.begin(); it != StockInventory.end(); it++)
        cout << it->first << "\t\t" << it->second.first << "\t" << it->second.second << endl;
    cout << "------------------------" << endl;
}

pair<int, double> OMS::get_internalization_price_and_qty(string tick){                                              // if we have the stock in our inventory, return the avg price
    pair<int, double> inv = StockInventory[tick];
    return (inv.first == 0) ? make_pair(0, numeric_limits<double>::max()) : inv;                                    // if we don't have a stock, qty = 0, price = inf
}

pair<float, int> OMS::get_best_mkt_price(Type &o){                                                                  // returns the index of the best ECN and best price available at that ECN
    int best_ecn_index = 0;
    float best_price;
    if(o == Bid){                                                                                                   // looking for lowest ask to buy into
        best_price = numeric_limits<float>::max();
        for(int i = 0; i < available_ECNs.size(); i++){
            if(available_ECNs[i]->market.get_best_ask_price() < best_price && !available_ECNs[i]->market.get_current_asks()->empty()){
                best_price = available_ECNs[i]->market.get_best_ask_price();
                best_ecn_index = i;
            }
        }
    }
    else{                                                                                                           // looking for highest bid to sell into
        best_price = -1;
        for(int i = 0; i < available_ECNs.size(); i++){
            if(available_ECNs[i]->market.get_best_bid_price() > best_price && !available_ECNs[i]->market.get_current_bids()->empty()){
                best_price = available_ECNs[i]->market.get_best_bid_price();
                best_ecn_index = i;
            }
        }
    }
    return make_pair(best_price, best_ecn_index);
}

pair<bool, string> OMS::eligible_order(Order &o){
    //this function will do all of the checks to ensure the account balance is sufficient, account is not suspended etc.
    int num_shares_owned;
    double price;

    vector<Account *>::iterator acc = Accounts.begin();
    for(acc; acc != Accounts.end(); acc++){
        if((*acc)->get_account_num() == o.get_account_num())
            break;
    }
    if(acc == Accounts.end())
        return make_pair(false, "Account not found");
    
    // ---------- Deal with cover order --------------------
    if(o.get_action() == Cover){
        pair<int, double> short_pos = (*acc)->get_short_pos(o.get_ticker());
        if(o.get_size() > abs(short_pos.first))
            o.set_size(abs(short_pos.first));                                                                   // if we try to cover a position larger than the one we have, change order size to our position size
        if(short_pos.first == 0)                                                                                // if client doesn't have a short position for this stock.
            return make_pair(false, "reason: cannot cover a short position if no short position exists");
    }
    
    // ---------- Deal with Buy, Sell and Short-sell orders --------------------
     if((*acc)->get_activity_status() == 1){                                                                    // if the account is active
         Type offer_type;
         (o.get_action() == Buy || o.get_action() == Cover) ? offer_type = Bid : offer_type = Ask;

        if(offer_type == Bid){                                                                                  // if it's a buy or cover order (which results in an offer of type Bid
            if(o.get_type() == Limit){                                                                           // for limit orders
                if((*acc)->get_balance() >= commission_rate + (o.get_size() * o.get_enter_price()))                               // ensure the client has a cash balance adequate w.r.t. their order price
                        return make_pair(true, "");
                else
                    return make_pair(false, " insufficient account balance");
            }
            
            else if(o.get_type() == Market) {                                                                    // if it's a market order to buy
                if((*acc)->get_balance() >= commission_rate + (o.get_size() * get_best_mkt_price(offer_type).first))  // check to see if we have enough cash w.r.t. current market conditions
                    return make_pair(true, "");
                else{                                                                                           // if we don't display some info and return false with reason
                    cout << "Market price ==== " << get_best_mkt_price(offer_type).first << endl;
                    cout << "Cash balance = " << (*acc)->get_balance() << endl;
                    return make_pair(false, "insufficient cash balance to fill market 'buy' order given current market conditions");
                }
            }
            else                                                                                                // for other order types
                return make_pair(false, "we are not processing orders other than Limit and Market at the moment");
        }
        else {                                                                                                  // if its a sell order, does this account own a sufficient number of shares?
            if(o.get_action() == Short)
                return make_pair(true, "");                                                                     // assume we always have shares to lend out

            if(o.get_action() == Sell){
                num_shares_owned = (*acc)->get_long_pos(o.get_ticker()).first;
                if(num_shares_owned == 0)
                    return make_pair(false, "we cannot sell any shares that we do not own, we can only short sell if we own no shares");
                
                if(num_shares_owned < o.get_size())                                                             // if you enter an order to sell X shares for ABC stock where X > (# of ABC shares owned), then sell all the shares owned of ABC
                    o.set_size(num_shares_owned);
            }
        }
    }
    else
        return make_pair(false, "Inactive account");

    return make_pair(true,"");                                                                                  // if we pass all cases, we're good to go
}


vector<Order>::iterator OMS::find_pending_order(int id){
    /*
     This returns an iterator pointing to the order in pending_orders which corresponds to the order_info object's id
    
     */
    vector<Order>::iterator order_iter = pending_orders.end();
    for(vector<Order>::iterator it = pending_orders.begin(); it != pending_orders.end(); it++)
        if(it->get_order_num() == id)
            return it;
    return order_iter;                                                                                          // return a dud iterator
}

vector<string> convert_enum_to_string(OrdType ord_type, Term ord_term, Action ord_act, Status ord_status, milliseconds time){
    string order_type, order_term, order_action, order_status, order_time;
    switch(ord_type){
        case 0:
            order_type = "Limit";
            break;
        case 1:
            order_type = "Market";
            break;
    }
    switch(ord_term){
        case 0:
            order_term = "AON";
            break;
        case 1:
            order_term = "GTC";
            break;
        case 2:
            order_term = "IOC";
            break;
        case 3:
            order_term = "FOK";
            break;
        case 4:
            order_term = "Day";
    }
    switch(ord_act){
        case 0:
            order_action = "Buy";
            break;
        case 1:
            order_action = "Sell";
            break;
        case 2:
            order_action = "Short";
            break;
        case 3:
            order_action = "Cover";
            break;
    }
    switch(ord_status){
        case 0:
            order_status = "Filled";
            break;
        case 1:
            order_status = "Pending";
            break;
        case 2:
            order_status = "Canceled";
            break;
        case 3:
            order_status = "Aborted";
            break;
    }

    order_time = "TIME";
    vector<string> result;
    result.push_back(order_type);
    result.push_back(order_term);
    result.push_back(order_action);
    result.push_back(order_status);
    result.push_back(order_time);
    return result;
}

void OMS::UpdateAccount(Order &o){
    // This function updates the order, contains stock and contains short table in the DBMS after the order has been filled and finalized.
    
    bool debug = false;
    vector<Account *>::iterator account = Accounts.begin();

    for(account; account != Accounts.end(); account++){                                                                     // get account that placed the order
        if((*account)->get_account_num() == o.get_account_num())
            break;
    }
    if(account == Accounts.end()){
        cout << "Error: account not found." << endl;
        return;
    }
    
    if(debug){
        cout << "Account balance in UpdateAccount: " << (*account)->get_balance() << endl;
    }
    
    //============================== UPDATE CASH BALANCE ==============================
    if(o.get_action() == Buy || o.get_action() == Cover)
        (*account)->set_balance((*account)->get_balance() - o.get_fill_price() * o.get_size() - commission_rate);           // deplete cash balance to appropriate level
    else
        (*account)->set_balance((*account)->get_balance() + (o.get_fill_price() * o.get_size()) - commission_rate);         // replenish cash

    cout << "OMS: Cash has been updated..." << endl;

    //============================== UPDATE ORDER TABLE ==============================
    if(o.get_type() == Market && (o.get_action() == Buy || o.get_action() == Cover))                               // if we changed o.Price to MAX FLOAT for processing purposes,
        o.set_price(NULL);                                                                                         // entered price cannot be stored as MAX FLOAT in DBMS and for market, change back to NULL

    (*account)->add_order(o);
    cout << "OMS: Order table has been updated..." << endl;

    //============================== UPDATE CONTAINS STOCK OR CONTAINS SHORT ==============================
    if(o.get_action() == Buy || o.get_action() == Sell){
        (*account)->update_contains_stock(o);
        cout << "OMS: Contains stock has been updated..." << endl;
    }
  
    else{                                                                                                           // we are updating ContainsShort and dealing with either 1. Short or 2. Cover order
        cout << "updating containsShort table" << endl;
        (*account)->update_contains_short(o);
  }
    cout << "\nHERE IS ORDER TABLE UPDATE: " << endl;
    o.Print();
}

void OMS::update_orders(vector<order_info> &vec){
    vector<Order>::iterator order_iter;                                                                             // iterator will point to the order which corresponds to the order_info being examined
    bool debug = false;

    for(vector<order_info>::iterator it = vec.begin(); it != vec.end(); it++){                                      // search through the order_info vector
        order_iter = find_pending_order(it->order_ID);                                                              // get the iterator which points to the order we're looking for in the vector of pending orders
        if(order_iter != pending_orders.end()){                                                                     // if the order that corresponds to the order_info object is in pending_orders (it should always be)
            if(it->offer_status == Pending){                                                                        // if the transaction which returned order_info iterator it did not fill the order completely
                if(it->avg_price > 0){                                                                              // see # 1
                    order_iter->set_fill_price(order_iter->get_fill_price() + it->avg_price * it->shares_filled);   // we will compute the avg fill price once the order is filled
                }
            }

            else if(it->offer_status == Filled){                                                                    // if the transaction which returned order_info iterator it has filled the order
                if(order_iter->get_fill_price() > 0){                                                               // if it was partially filled first, then filled completely
                    order_iter->set_fill_price(order_iter->get_fill_price() + (it->avg_price * it->shares_filled) / order_iter->get_size());    // compute avg price
                }
                else                                                                                                // the order was filled completely from the getgo
                    order_iter->set_fill_price(order_iter->get_fill_price() + it->avg_price);

                order_iter->set_status(Filled);                                                                     // change order status in pending_orders to filled

                if(debug) {                                                                                         // for debugging & verification purposes
                    cout << "\nOMS: erasing order number: " << order_iter->get_order_num() << " from pending orders" << endl;
                    cout << "Before erasing, here's the order_info of that order: " << endl;
                    it->Print();
                }

                UpdateAccount(*order_iter);                                                                         // make appropriate changes to OrderTable in the DBMS (for this local copy, it updates Account objects)
                cout << "\nOrder erased from pending orders..." << endl;
                pending_orders.erase(order_iter);                                                                   // eliminate the filled order from pending_orders
            }
        }
        else {
            cout << "ERROR: NO MATCHING PENDING ORDER" << endl;                                                     // catch errors
        }
    }
}

/* update_orders
if order was added but couldn't be filled at all in the ECN, the ECN sets order_info.avg_price = -1. So in this case, there is nothing to be udpated with respect to this order
 */

order_info OMS::internalize_order(Offer &o, pair<int, double> internal_inventory, float i_price, string tick){              // called after all checks have been confirmed that we should internalize
    vector<order_info> temp_order_info;
    
    if(internal_inventory.first >= o.Qty){                                                                                  // if broker can completley fill order
        internal_inventory.first -= o.Qty;                                                                                  // update internal_inventory quantity
        temp_order_info.push_back(order_info(i_price, o.Qty, Filled, o.OrderID, false));                                    // generate order_info for this order
        internalization_profit += i_price * o.Qty;
    }

    else {                                                                                                                  // broker can partially fill order using all of our inventory
        temp_order_info.push_back(order_info(i_price, internal_inventory.second, Pending, o.OrderID, false));
        internalization_profit += i_price * internal_inventory.first;
        internal_inventory.first = 0;                                                                                       // updating internal inventory quantity to 0
    }

    if(internal_inventory.first > 0)                                                                                        // if stock_inventory has remaining shares
        StockInventory[tick] = internal_inventory;
    else
        StockInventory.erase(tick);                                                                                                 // otherwise remove the position from inventory

    return temp_order_info[0];                                                                                                      // return order_info for order that was internalized
}


vector<order_info> OMS::ProcessOrder(Order &o){
    vector<order_info> order_information;
    pair<bool, string> eligibility_info = eligible_order(o);
    bool debug = false;
    
    if(eligibility_info.first){                                                                                                     // if our order is eligible to be filled
        bool send_to_ecn = true;                                                                                                    // flag indicates if we are routing to ecn. default value = true
        // ============================ CONVERTING ORDER TO OFFER ============================
        if(o.get_type() == Market &&(o.get_action() == Sell || o.get_action() == Short))
            o.set_price(0);                                                                                                         // Set price = 0 for sell orders cause market will dictate the fill price
        else if(o.get_type() == Market && (o.get_action() == Buy || o.get_action() == Cover))
            o.set_price(std::numeric_limits<float>::max());                                                                         // Set price = MAX for buy market orders cause, again, price is determined by the market

        pending_orders.push_back(o);                                                                                                // add to pending orders
        Offer new_offer(o.get_enter_price(), o.get_size(), o.get_time(), o.get_term(), o.get_type(), true, o.get_order_num());      // true means the offer was placed from the OMS
        (o.get_action() == Buy || o.get_action() == Cover) ? new_offer.OfferType = Bid : new_offer.OfferType = Ask;                 // setting appropriate offer type

        //============================ DECIDING IF INTERNALIZING IS PROFITABLE && POSSIBLE ============================ (see Internalization info section below)
        pair<int, double> internal_inventory = get_internalization_price_and_qty(o.get_ticker());                                    // see #2 of Internalization
        pair<float, int> best_mkt_price_and_location = get_best_mkt_price(new_offer.OfferType);                                     // get best price in the market and location of that price

        bool internalization_is_profitable = (new_offer.OfferType == Bid) ? (internal_inventory.second < o.get_enter_price()) : false;                         // Note: see #3
        bool internalization_is_viable = (new_offer.OfferType == Bid) ? (internal_inventory.second < best_mkt_price_and_location.first) : false;               // see #4
        
        if(internalization_is_viable && internalization_is_profitable){                                                             // we internalize the order if it is both profitable and viable to do so
            cout << "OMS: Internalizing order..." << endl;
            DisplayInventory();                                                                                                     // display broker's inventory before internalization
            
            float internalization_price;
            (o.get_type() == Market) ? internalization_price = best_mkt_price_and_location.first - .01 : internalization_price = o.get_enter_price();

            order_info internalization_info = internalize_order(new_offer, internal_inventory, internalization_price, o.get_ticker());
            if(debug){
                cout << "\n Internalization info: \n";
                internalization_info.Print();
            }

            if(internalization_info.offer_status == Filled)
                send_to_ecn = false;                                                                                                // do not send to ecn if we filled the order
            
            cout << "OMS: Internalization complete..." << endl;
            DisplayInventory();                                                                                                     // display broker's inventory after internalization
            order_information.push_back(internalization_info);                                                                      // add internalization info to order_info vec
        }

        else if(send_to_ecn){                                                                                                       // we are not internalizing order

            // ============================ SENDING OFFER TO BEST ECN ============================
            pair<float, int> best_mkt_price_and_location = get_best_mkt_price(new_offer.OfferType);                                 // for test purposes
            cout << "OMS: routing order..." << endl;
            order_information = this->available_ECNs[best_mkt_price_and_location.second]->ParseOffer(new_offer);                    // routing order. see #3 Below
            int info_size = order_information.size();
            cout << "OMS: has retrieved order_info..." << endl;
            
            if(debug){
                cout << "ORDER INFO RECEIVED: " << endl;
                if(info_size == 0)
                    cout << "No order info received!" << endl;
                else{
                    for(auto i : order_information)
                        i.Print();
             }
         }

            // ============================ REROUTING INCOMPLETE MARKET ORDER ============================
            //remember: order_information.end() points to the order being processed
            int count = 0;                                                                                                  // number of reroutings
            if(debug)
                cout << "order_information.end incomplete market order = " << order_information[info_size - 1].incomplete_market_order << endl;
            while(o.get_type() == Market && order_information[info_size - 1].incomplete_market_order == true){              // while market order is incomplete
                cout << "Rerouting incomplete market order..." << endl;
                cout << "here is order info before rerouting attempt " << count + 1 << endl;                                // display num of rerouting
                if(!order_information.empty()){
                    for(auto i : order_information)
                        i.Print();
                }
                else
                    cout << "No order_info to display" << endl;

                new_offer.Qty -= order_information.end()->shares_filled;                                                    // update new_offer qty before rerouting
                best_mkt_price_and_location = get_best_mkt_price(new_offer.OfferType);                                      // scan market and find ECN with best pricing opportunity
                order_information = this->available_ECNs[best_mkt_price_and_location.second]->ParseOffer(new_offer);        // send order to ECN
                if(++count > 100){                                                                                          // mostly for debugging purposes throughout development
                    cout << "ERROR: Incomplete market order timed out!" << endl;                                            // prevents infinite loop if for some reason the order cannot be filled given the order parameters
                    break;
                }
            }
        }

        if(order_information.size() > 0)
            update_orders(order_information);                                                                               // use order_info object to update account info
     }

    else {
        cout << "Order " << o.get_order_num() << " Ineligible: " << eligibility_info.second << endl;                        // if the order is ineligible, display the reason why
        o.set_status(Aborted);
    }
    return order_information;
}
/*
 1. The need for converting ECN to an Offer:
        The OMS and the various ECNs are separate entities. I created a simple 'protocol' to pass information between them. The OMS sends messages to the ECN in packets of information called offers. These offers encapsulate the information necessary for the ECN to process the Order. We don't submit Order objects to the ECN because Orders contain confidential data (e.g. account numbers) and other extraneous information.
 
 
 2.  The need for rerouting incomplete market orders:
        An exchanges level of liquidity varies throughout the day. At certain times (e.g. early in the morning) markets are very illiquid. Sometimes market orders will be routed to an exchange, filled partly, and chew up all of the liqudiity that was available at that exchange. If this happens, the exchange will send the market offer back to the OMS to be rerouted to another exchange. The OMS will route the market order to another ECN w/a sufficient level of liqudity to fulfill the rest of the order.
 
 3. Order_info objects - the other side of the coin:
 
    Background: What is "order_information = this->available_ECNs[best_mkt_price_and_location.second]->ParseOffer_TEST(new_offer);" doing?
 
     best_mkt_price_and_location contains the index of vector<class ECN *> that corresponds to the ECN with the current best price in the market for the order which was submitted. Since this vector is a member of OMS, we use this-> to access it, then index it to access the ECN with the best price. Now that we're indexing the ECN with the best price, we call the function ParseOffer() to submit the offer from the OMS to the ECN. The ECN then does its thing and returns a vector of order_info objects. The offer objects are used to communicate form the OMS to the ECN. Conversely, the order_info objects are used to send information from an ECN to the OMS pertaining to the offer that was parsed. This information is then used to update the account and finalize the processing of the order.
 */



/*
 1. Logic of Internalization:
     Consider the scenario where we own 500 shares of IBM which we obtained for $100 / share. Suppose we want to sell these shares whenever we, the broker, can do so at a profit (for internalization purposes).

     The DBMS table "Stock Inventory" contains the shares of stocks the broker is willing to internalize for a profit as well as the price that was paid to obtain those shares.

     Situation: An incoming buy (limit) order is being processed with a qty of 1000 shares and a price of $110 share. The best market price at the moment is $110.05.

     - this will also internalize offers which have a buy (market) order where the price paid (in inventory) is less than the market price. Then we will fill them at $0.01 below the current best price in the market.

     The OMS will assess the situation as follows:
     - we are willing to sell 500 of our shares to the client who has placed this order because we will earn a profit of 500 * 110 = $55000 AND this provides a better price for those 500 shares than what is available currently in the market. (If this was a market order, the OMS will buy them for $110.04 because we are no longer bound to the constraint of providing a price better than or equal to the price specified by a limit order)

     - Given this, the OMS will fill 500 shares of the order at $110/share using the shares in the broker_inventory. Then it will reduce the quantity of shares in the broker inventory via a SQL query.

     - After filling all or part of the order via internalization, we will need to update_pending_orders() given the order_info that results from this internalization

     - The OMS then checks if any shares are remaining in the order (there are 500 remaining in this case) and will route the order to an ECN for fulfillment.

     - After the order has been filled at the ECN w/ best market price, we will update the pending_orders()

 2. get internalization price, (it will be MAX FLOAT if we cannot internalize it)
        internal_inventory.first = price paid for security by my_broker (obtained from DBMS)
        internal_inventory.second = qty of inventory the broker is willing to internalize (which is obtained from DBMS as well)

 3. Note: right now, we are only internalizing incoming Bids by comparing them to our inventory. In order to determine if we can internalize an incoming Ask we will need to assess all other orders to be processed by the OMS and determine if it is advantageous to buy the shares off the client. Other methods of determining whether to internalize an incoming sell order are beyond the scope of this project.

 4. This function answers the question: is internalizing the order viable given the responsibility of providing the best possible price? It is incumbent on the stock broker to provide the best possible price for a security given current market conditions, so we can only internalize if we meet these constraints.

 */








