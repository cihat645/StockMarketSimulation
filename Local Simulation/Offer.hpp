//
//  Offer.hpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 8/18/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#ifndef Offer_hpp
#define Offer_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <chrono>
#include <iomanip>
#include "Order.h"


using namespace std;
using namespace chrono; 


/* ======================================= ABOUT OFFER =========================================================================
Offer objects are used to create Bids and Asks which from customer orders and the data we are streaming from disk. These offers are then submitted from the OMS to an ECN when fulfilling an order. I have composed this into one class with a "Type" attribute to specify whether the offer is a bid or ask since the composition of a bid is identical to an ask (with the exception of the offer type).

 I initially created the offer instead of submitting orders directly to the ECN because orders contain sensitive information that is not needed for the ECN to process it.

 my_offer:
     boolean value indicating if the offer in the system originated from "my broker". When an offer is used to form a transaction in the ECN, a report must be sent back to the Broker who submitted that order giving various details (e.g. average order price, quantity filled). In reality, these reports are sent to numerous brokers. In this simulation, there is only one broker, "my broker". This value will be utilized by the ECN to submit the rest of the order information for partially filled orders (orders that were filled partially at the time of processing, but then could not continue to be filled due to the parameters of the order. They are then added to current_bids or current_asks. Once the market conditions meet the constraints of the order, it will be removed from current_bids (current_asks). At this time, the ECN will check this value. If it's true, it will submit the necessary order details to the broker, if it's false (which it will be for all offers pulled from the data set), it won't submit anything.
==============================================================================================================================*/

enum Type{Bid, Ask};

class Offer{
private:
    
public:
    Type OfferType;
    float Price;
    int Qty, OrderID;
    milliseconds timestamp;
    Term OfferTerm;
    OrdType OrderType;
    bool my_offer;
    string exchange;
    void Print(){
        cout << "-------------------------------" << endl;
        cout << "| " << Price << setw(7) << Qty << setw(15) << timestamp.count() << "|" << endl;
        cout << "-------------------------------" << endl;
    }

    Offer(){
        Price = 0.0;
        Qty = 0;
        OfferType = Bid;
        timestamp = milliseconds(1);
        my_offer = false;
    }
    
    Offer(float p, int q, milliseconds m, Term t, OrdType ot, bool b, int oID){
        Price = p;
        Qty = q;
        OrderID = oID;
        timestamp = m;
        OfferTerm = t;
        OrderType = ot;
        my_offer = b;                                                                                               //indicates whether the offer was placed by a client using the app or was pulled from the data set
    }
    
    Offer(float P, int Q, Type t, milliseconds ms){                                                                 // used in create_offer_with_dest
        Price = P;
        Qty = Q;
        OfferType = t;
        timestamp = ms;
        OrderType = Limit;
        my_offer = false;
    }
    
    Offer(float P, int Q, Type t, milliseconds ms, string s ){                                                      // used in create_offer_with_dest
        Price = P;
        Qty = Q;
        OfferType = t;
        timestamp = ms;
        OrderType = Limit;
        exchange = s;
        my_offer = false;
        OfferTerm = Day; // default day order term
    }
};
#endif /* Offer_hpp */
