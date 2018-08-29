//
//  ECN.cpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 8/16/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

// ORDER MATCHING ALROGITHMS: PRO RATA & Time / Price Priority (FIFO)
//https://stackoverflow.com/questions/13112062/which-are-the-order-matching-algorithms-most-commonly-used-by-electronic-financi

// There are several order matching algorithms. We will implement the FIFO algorithm which is used by most nasdaq groups (source: https://docs.google.com/viewer?a=v&pid=sites&srcid=ZGVmYXVsdGRvbWFpbnxyYWplZXZyYW5qYW5zaW5naHxneDo2ZWE1YjRhYzQxZWIyYWQx)

#include <algorithm>
#include <iostream>
#include <tgmath.h>
#include <vector>
#include "ECN.hpp"
#include "Order.h"
#include "Offer.hpp"

using namespace std;

void Market::insert_front(Offer &o){
    (o.OfferType == Bid) ? current_bids.insert(current_bids.begin(), o) : current_asks.insert(current_asks.begin(), o);
}

void Market::remove_best_bid(){
    if(!current_bids.empty()) current_bids.erase(current_bids.begin());
}

void Market::remove_best_ask(){
    if(!current_asks.empty()) current_asks.erase(current_asks.begin());
}

bool compare_bids(Offer const b1, Offer const b2){
    if(b1.Price != b2.Price)
        return b1.Price > b2.Price;
    else
        return b1.timestamp < b2.timestamp;
}

bool compare_asks(Offer const a1, Offer const a2){
    if(a1.Price != a2.Price)
        return a1.Price < a2.Price;
    else
        return a1.timestamp < a2.timestamp;
}

void Market::insert_offer(Offer &o){
    /*
     Inserts an offer into the market. We want best_bid and best_ask to be stored at current_bids[0], current_asks[0]. We insert bids and asks systematically to achieve this.
     iteration_bounds.first = iterator of first offer that meets the condition w/respect to o.Price
     iteration_bounds.second = iterator of the first offer that does NOT meet the condition w/respect to o.Price
     */
    
    pair<vector<Offer>::iterator, vector<Offer>::iterator> insertion_bounds;
    if(o.OfferType == Bid) {
        insertion_bounds = equal_range(current_bids.begin(), current_bids.end(), o, compare_bids);
        current_bids.insert(insertion_bounds.first, o);
    }
    else {
        insertion_bounds = equal_range(current_asks.begin(), current_asks.end(), o, compare_asks);
        current_asks.insert(insertion_bounds.first, o);
    }
}

void Market::Print_Offers(Type t){
    if(t == Bid){
        for(int i = 0; i < current_bids.size(); i++)
            current_bids[i].Print();
    }
    else {
        for(int i = 0; i < current_asks.size(); i++)
            current_asks[i].Print();
    }
    cout << "\n" <<  endl;
}



//======== make_transaction() ================================================================================================================
//The make_transaction functions are responsible for completing a transaction with a bid or ask that has just been parsed by the ECN.
// this includes:
// - removing ask offers from current_asks if o.OfferType = Bid
// - removing bid offers from current_bids if o.OfferType = Ask
// - recording the information needed by the OMS regarding the order
//=============================================================================================================================================


vector<order_info> ECN::make_transaction_ask(Offer &o){                 // makes a transaction for an offer of type 'Ask'
    /*
     This function takes in an ask offer, fills and shares of the offer if feasible and accumulates data on the offers that were used to fill it if those offers were submitted by a client of the OMS (us).
     
     input:     offer - the offer that is causing this transaction to occur
     returns:   transaction_return_data - this is a series of order_info objects that correspond to each bid and ask that were used in creating this transaction.
     */
    vector<order_info> transaction_return_data;                         // holds the order_info objects
    num_of_transactions++;                                              // increment number of transactions for this ECN
    Offer best_bid = market.get_best_bid();                             // obtain current best bid for IBM stock on this ECN
    int order_qty = o.Qty;                                              // the quantity of shares that have been filled (does not reflect it rn, but it will at the end of function call)
    float avg_price = 0;                                                // avg price of transaction for offer o
    Status o_stat = Pending;
    bool market_order_flag = false;                                     // flag = true if there is insufficient market liquity for market order to be filled and it should be rerouted

    if(o.OfferTerm == GTC || o.OfferTerm == Day){                                                   // Deal with GTC and Day order types
        while(o.Price <= best_bid.Price && o.Qty > 0 && !market.get_current_bids()->empty()){       // while bid can be filled by this offer
            if(o.Qty >= best_bid.Qty){                                                              // if we can eliminate the best bid completely with this ask offer
                avg_price += best_bid.Price * best_bid.Qty;                                         // use that bid to update avg price info
                if(best_bid.my_offer)                                                               // if the best_bid was placed by my broker, create an order_info object and add it to the vector
                    transaction_return_data.push_back(order_info(best_bid.Price, best_bid.Qty, Filled, best_bid.OrderID, market_order_flag));   // if the bid used was submitted from us, we need to get that information
                market.remove_best_bid();                                                           // remove best bid
            }
            else{                                                                                   // cannot eliminate the entire best_bid, but can fill the ask order we are processing
                avg_price += best_bid.Price * o.Qty;                                                // multiplying by o.Qty because we are not eliminating the entire best_bid
                market.current_bids[0].Qty -= o.Qty;                                                // reduce best bid qty
                if(best_bid.my_offer)                                                               // again, if the bid used was submitted from us, we need to get that information and return it to update the account that submitted that order
                    transaction_return_data.push_back(order_info(best_bid.Price, o.Qty, Pending, best_bid.OrderID, market_order_flag)); // Pending status with a positive quantity of shares filled signifies that the bid was partially filled
            }
            o.Qty -= best_bid.Qty;                                                                  // o.Qty = # shares left to be filled for our order
            best_bid = market.get_best_bid();                                                       // get next best bid
        }

        if(o.Qty > 0){                                                                              // if ask has any shares left to fill, but can't be filled at the moment, update share qty and add to current_asks, unless it's a market order - which we then flag.
            if(o.OrderType == Market){                                                              // if it's a market order we don't want to add it to current_asks of the ECN
                market_order_flag = true;                                                           // if there are still shares left to be filled, but insufficient liquidity, flag it!
                cout << "o.qty = " << o.Qty << endl;
                cout << "FLAGGING MARKET ORDER AS INCOMPLETE" << endl;
             }
            else
                market.insert_offer(o);                                                             // else insert the limit order to current_asks so it can eventuallly be filled
            order_qty -= o.Qty;                                                                     // order_qty initialized to total num shares. shares filled = total num shares - shares left to be filled
        }
        else
            o_stat = Filled;                                                                        // if there are no shares left to be filled, change the order status to 'Filled'

        avg_price /= order_qty;                                                                     // compute avg price
        if(o.my_offer)                                                                              // if the ask was submitted by us, then we are going to add it to the order info vector
            transaction_return_data.push_back(order_info(avg_price, order_qty, o_stat, o.OrderID, market_order_flag));  // adding the order_info for the ask we are processing
    }

    /*  DEAL WITH IOC, AON & FOK OFFER TYPES - Feel free to add some code here!
        else { // o.OfferTerm == AON || IOC || FOK
    
    
        }
    */
    return transaction_return_data;                                                                 // last element of transaction_return_data contains order_info for the order currently being processed
}

vector<order_info> ECN::make_transaction_bid(Offer &o){
    /*
     This function is very similar to the make_transaction_ask function above. I originally had these functions merged into one, but the code was getting a little convoluted so I split them into two functions.
     It takes in an bid offer, fills and shares of the offer if feasible and accumulates data on the offers that were used to fill it if those offers were submitted by a client of the OMS (us).
     
     input:     offer - the offer that is causing this transaction to occur
     returns:   transaction_return_data - this is a series of order_info objects that correspond to each bid and ask that were used in creating this transaction. Only add info for bids and asks that were submitted by us.
     */
    vector<order_info> transaction_return_data;                                                                     // holds the order_info for every bid and ask that was used to create the transaction
    num_of_transactions++;
    Offer best_ask = market.get_best_ask();
    int order_qty = o.Qty;                                                                                          // the quantity of shares that have been filled
    float avg_price = 0;                                                                                            // avg price of transaction for offer o
    Status o_stat = Pending;
    bool market_order_flag = false;                                                                                 // flag = true if there is insufficient market liquity for market order to be filled and it should be rerouted

    if(o.OfferTerm == GTC || o.OfferTerm == Day){                                                                   // Deal with GTC and Day offers
        while(o.Price >= best_ask.Price && o.Qty > 0 && !market.get_current_asks()->empty()){                       // while bid can be filled by best ask
            if(o.Qty >= best_ask.Qty){                                                                              // can eliminate the best ask completely with the bid order
                avg_price += best_ask.Price * best_ask.Qty;                                                         // update average price
                if(best_ask.my_offer){                                                                              // if the best_ask was placed by our broker, create an order_info object and add it to the vector
                    order_info temp(best_ask.Price, -1, Filled, best_ask.OrderID, market_order_flag);               // creating order_info for best_ask being filled
                    transaction_return_data.push_back(temp);                                                        // add order info to vector
                }
                market.remove_best_ask();                                                                           // remove best ask
            }
            else{                                                                                                   // cannot eliminate the entire best_ask, but can complete the bid order we are processing
                avg_price += best_ask.Price * o.Qty;                                                                // update average price
                market.current_asks[0].Qty -= o.Qty;                                                                // reduce best ask qty

                if(best_ask.my_offer){                                                                              // if the best_ask originated from my broker OMS, then create order_info obj
                    order_info temp(best_ask.Price, o.Qty, Pending, best_ask.OrderID, market_order_flag);           // signifies that the bid was partially filled
                    transaction_return_data.push_back(temp);                                                        // add the order info to the return vector
                }
            }
            o.Qty -= best_ask.Qty;                                                                                  // o.Qty = # shares left to be filled
            best_ask = market.get_best_ask();                                                                       // get best ask
        }

        if(o.Qty > 0){                                                                                              // if ask has any shares left to fill, but can't be filled at the moment, update share qty and add to current_asks
            if(o.OrderType == Market)                                                                               // deal with market offer
                market_order_flag = true;                                                                           // if there are still shares left to be filled, but insufficient liquidity, flag it!
            else
                market.insert_offer(o);                                                                             // else insert the limit order to current bids
            order_qty -= o.Qty;                                                                                     // order_qty initialized to total num shares. shares filled = total num shares - shares left to be filled
        }
        else
            o_stat = Filled;                                                                                        // set order status

        avg_price /= order_qty;                                                                                     // compute final average price
        if(o.my_offer)
            transaction_return_data.push_back(order_info(avg_price, order_qty, o_stat, o.OrderID, market_order_flag));  // adding the order_info for the ask we are processing
    }

    // else {process FOK, IOC, etc.}                                                                                // didn't have time to implement these order types
    return transaction_return_data;                                                                                 // last element of transaction_return_data contains order_info for the order being processed
}

vector<order_info> ECN::ParseOffer(Offer &o){
    /*
     This function processes all offers that are submitted to the ECN. In our simulation, we send all of the market offers to each ECN via this function as well as our own offers.
     */
    float threshold = 0;                                                                            // for now, the threshold = 0, this is the spread or "cut" the ECN will take for each transaction
    vector<order_info> order_info_vec;                                                              // create a vector of order_info objects
    if(o.my_offer)                                                                                  // if the offer has been submitted from us,
        cout << "ECN: Processing my order at " << ECN_Name << "..." << endl;                        // notify user the order is being processed
    
    if(o.OfferType == Ask){
        if(o.OrderType == Market){                                                                  // if dealing with a market offer
            if(!market.get_current_bids()->empty())  {                                              // we need to ensure that there is sufficient market liquidity
                cout << "Order info vector coming from ParseOffer_test(make_transaction_ask_NEW())" << endl;
                return make_transaction_ask(o);                                                     // use this market order to create a transaction in the market
            }
        }

        else if(o.OrderType == Limit){                                                                              // if dealing with a limit offer
            if(o.Price - threshold <= this->market.get_best_bid().Price && !market.get_current_bids()->empty())     // if offer o can make a transaction happen
                return make_transaction_ask(o);
            else
                this->market.insert_offer(o);                                                                       // otherwise add this offer to the offer book
        }
    }

    else {                                                                                                          // If we're processing a Bid
        if(o.OrderType == Market){                                                                                  // and it's a market offer
            if(!market.get_current_asks()->empty())                                                                 // we need to ensure that there is sufficient market liquidity
                return make_transaction_bid(o);                                                                     // if there is, use this market offer to create a transaction
        }
        else if(o.OrderType == Limit){                                                                              // otherwise, it's a limit bid
            if(o.Price + threshold >= this->market.get_best_ask().Price && !market.get_current_asks()->empty())     // if offer o can make a transaction happen and there's liquidity in the market
                return make_transaction_bid(o);                                                                     // make transaction happen
            else
                this->market.insert_offer(o);                                                           // add limit offer to current bids
        }
    }
    
    if(o.my_offer)                                                                      // only return this info if the offer was submitted by us.
        order_info_vec.push_back(order_info(-1, 0, Pending, o.OrderID, false));         // if we get to this point, the order was not immediately used to create a transaction, so we return a pending order_info struct
    return order_info_vec;                                                              // the price of -1 and shares filled of 0 indicate that the order has not been filled at all yet
}

 




/* ================= PARSING DATA ==================
 
 - Due to the inconsistent formatting of the TAQ (Trade and Quote) data we have obtained, we will initally be processing the data as follows:

   Event types "QUOTE BID NB" and "QUOTE BID" will be treated the same. As will "QUOTE ASK NB" and "QUOTE ASK".

   We will ignore "TRADE" and "TRADE NB" event types because there is no information pertaining to which offers are being cleared when the trade takes place. In addition, the "TRADE" and "TRADE NB" rows are trades at a specific exchange. Since we are initially treating the entire file as one exchange, it doesn't make sense to process these offers. NOTE: it would be interesting to record all the trade informaiton as the data is being processed by the ECN and compare the trade results from our ECN to the trade tuples in the file.

 We will commence the ECN's functionality by simply using the insert_offer function to handle all offers in the file.


 100.1      200     9:06

 100.1      100     9:00 //BB
 100.1      299     9:05
 100.1      500     9:08
 100.05     250     8:57
 100.03     500     8:34
*/
