//
//  Offer.hpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 3/18/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#ifndef Offer_hpp
#define Offer_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <chrono>
#include "Order.h"

using namespace std;
using namespace chrono; 

enum Type{
    Bid, Ask
};

// ======================================= ABOUT OFFER =========================================================================
// The class "Offer" is used to create Bid and Ask objects which are generated from customer orders and sent to the ECN. I have composed this into one class with a "Type" attribute to specify whether the offer is a bid or ask since the composition of a bid is identical to an ask (with the exception of the offer type).

// I could potentially modify this to a template class and eliminate the OfferType attribute, but I'm not sure this is advantageous.

// we have an offer class so we can transcribe the necessary details to fill the order in a format to send to the ECN for order fulfillment. However, we might be able to eliminate the Offer class completely by making the attributes of 'Order' that we don't want the ECN to have access to (e.g. Account Number) private in the order class.
// ==============================================================================================================================

class Offer{
private:
    Type OfferType; // Bid or ask
    float Price;
    int Qty, OrderID; // Qty = qty of shares in order,  OrderID = OrderNumber of order, this is used to communicate order_info between the ECN and OMS
    milliseconds timestamp;
    Term OfferTerm; // AON, FOK, GTC,Day etc.
    string exchange;
    OrdType OrderType; // Market, limit, trailing etc.
    bool my_offer; // boolean value indicating if the offer in the system originated from "my broker". When an offer is used to form a transaction in the ECN, a report must be sent back to the Broker who submitted that order giving various details (e.g. average order price, quantity filled). In reality, these reports are sent to numerous brokers. In this simulation, there is only one broker, "my broker". This value will be utilized by the ECN to submit the rest of the order information for partially filled orders (orders that were filled partially at the time of processing, but then could not continue to be filled due to the parameters of the order. They are then added to current_bids or current_asks. Once the market conditions meet the constraints of the order, it will be removed from current_bids (current_asks). At this time, the ECN will check this value. If it's true, it will submit the necessary order details to the broker, if it's false (which it will be for all offers pulled from the data set), it won't submit anything.
public:

    float getPrice() {return Price; }
    Type getOfferType(){return OfferType;}
    milliseconds getTime(){return timestamp;}
    int getOfferQty() {return Qty;}
    Term getOfferTerm() {return OfferTerm;}
    bool getMyOffer() {return my_offer;}
    int getOrderID() {return OrderID;}
    void SetQty(int q) {Qty = q;}
    OrdType getOrderType(){return OrderType;}
    
    void Print();
    
    Offer(){ // default constructor
        Price = 0.0;
        Qty = 0;
        OfferType = Bid;
        timestamp = milliseconds(1);
        my_offer = false;
    }
    
    Offer(float p, int q, milliseconds m, Term t, OrdType ot, bool b, int oID){ // OMS constructor
        Price = p;
        Qty = q;
        OrderID = oID;
        timestamp = m;
        OfferTerm = t;
        OrderType = ot;
        my_offer = b; //indicates whether the offer was placed by a client using the app or was pulled from the data set
    }
    
    Offer(float P, int Q, Type t, milliseconds ms, string s ){ // for testing out bids and asks in the ECN WITH DESTINATION
        Price = P;
        Qty = Q;
        OfferType = t;
        timestamp = ms;
        OrderType = Limit;
        exchange = s;
        my_offer = false;
        OfferTerm = Day; // default day order term
    }
    
    void Offer::Print(){
        cout << "-------------------------------" << endl;
        cout << "| " << Price << setw(7) << Qty << setw(15) << timestamp.count() << "|" << endl;
        cout << "-------------------------------" << endl;
    }
};
#endif /* Offer_hpp */
