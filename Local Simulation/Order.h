//
//  Order.h
//  DBMS_Playground
//
//  Created by Thomas Ciha on 8/15/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#ifndef Header_h
#define Header_h

#include <string.h>
#include <chrono>
#include <iostream>

using namespace std;
using namespace chrono;

/* Order Term Descriptions: source: http://optionstradingbeginner.blogspot.ca/2009/01/fill-or-kill-fok-immediate-or-cancel.html

 FOK: Fill-Or-Kill (FOK)
 Fill-Or-Kill (FOK) order is an order (buy / sell) that must be immediately filled entirely (usually within a few seconds) at the limit price or better; otherwise, it will be totally cancelled. So, this order does not allow partial execution.

 FOK orders are normally used by day traders who are hoping to scalp or take advantage of the opportunity in the market within a short duration.

 IOC: Immediate-Or-Cancel (IOC)
 Immediate-Or-Cancel (IOC) order is an order that must be filled immediately at the limit price or better only. If the order cannot be filled immediately or fully (i.e. only partially filled), the unfilled portion will be cancelled.

 This order is different from Fill-Or-Kill (FOK) order whereby this order allows partial filling, while Fill-Or-Kill (FOK) order does not allow partial filling.

 AON: All-Or-None (AON) order is an order (buy / sell) that instruct the broker to either fill the order entirely at once at the limit price or better, or do not fill it at all.

 The difference between All-Or-None (AON) order and Fill-Or-Kill (FOK) / Immediate-Or-Cancel (IOC) orders is that, unlike FOK / IOC order, AON order will not be cancelled if it cannot be filled immediately, and can be used in addition to Day Order or Good Till Cancelled (GTC) order.
 If the order is a Day Order, when there is not enough supply to meet the quantity requested by the order at the limit price or better, then the order will be cancelled at the close of the trading day.

 GTC: GTC (Good til' Cancelled) Orders stay in the current_bids or current_asks until the order is canceled by the person who created it.

 DAY: Day orders are valid in the current_bids or current_asks until the end of the day. For the purposes of this project, they will be the default order term and will be canceled upon market close (4:30 pm)

*/

/* Here is a breakdown of how each order type will be dealt with in our application:

 Limit Order: limit orders guarantee your order will be filled at the specified price or better. They are essentialy stop orders, except they aren't converted into market orders once the price point is hit.

 Market Order: I think this is the easiest to deal with. The OMS sends a market order type and it is filled at the best ask or best bid.

 Market on Close: This simply submits a market order upon the market's close.

 Stop on Quote: this order will be converted into a market order once the market price breaches the "stop price", that is, the price at which you would like to buy or sell

 Stop Limit on Quote: TBD

 Hidden Stop: TBD

 Trailing Stop (Dollar): TBD

 Trailing Stop (Percent): TBD

 */


enum OrdType{Limit, Market};
enum Term{AON, GTC, IOC, FOK, Day};
enum Action{Buy, Sell, Short, Cover};
enum Status{Filled, Pending, Canceled, Aborted};

class Order{
private:
    double OrderNumber, AccountNumber;
    float Price, fill_price;
    int Size;
    string Ticker;
    milliseconds timestamp;
    OrdType OrderType;
    Term OrderTerm;
    Action OrderAction;
    Status OrderStatus;
    
public:
    int get_size(){return Size;}
    float get_enter_price() {return Price;}
    float get_fill_price() {return fill_price;}
    string get_ticker() {return Ticker;}
    OrdType get_type() {return OrderType; }
    Term get_term() { return OrderTerm; }
    Action get_action() { return OrderAction; }
    Status get_status() { return OrderStatus; }
    milliseconds get_time() {return timestamp; }
    double get_order_num() {return OrderNumber;}
    double get_account_num() {return AccountNumber;}
    
    void set_size(int s) {Size = s;}
    void set_price(float p) {Price = p;}
    void set_fill_price(float fp) {fill_price = fp;}
    void set_status(Status s) {OrderStatus = s;}
    
    
    Order(long ON, long AN, int S, string T, milliseconds Time, OrdType Ty, Term Trm, Action Act, float price){
        OrderNumber = ON;
        AccountNumber = AN;
        Size = S;
        Ticker = T;
        timestamp = Time;
        OrderType = Ty;
        OrderTerm = Trm;
        OrderAction = Act;
        Price = price;
        fill_price = 0;
        OrderStatus = Pending;
    }
    
    void Print(){
        cout << "-------- Order ID: " << OrderNumber << " -------- " << endl;
        cout << "Fill Price: " << fill_price << endl;
        cout << "Entered Price: " << Price << endl;
        cout << "Size: " << Size << endl;
        cout << "Timestamp: " << timestamp.count() << endl;
        cout << "Order Action: ";
        switch(OrderAction){
            case 0: cout << "Buy" << endl;
                break;
            case 1: cout << "Sell" << endl;
                break;
            case 2: cout << "Short" << endl;
                break;
            case 3: cout << "Cover" << endl;
                break;
        }
        cout << "Order Type: ";
        switch(OrderType){                                                                  // NOTE: there are several more order types, rn only dealing with limit and market orders
            case 0: cout << "Limit" << endl;
                break;
            case 1: cout << "Market" << endl;
                break;
            case 2: cout << "Market on Close" << endl;
                break;
            case 3: cout << "Stop on Quote" << endl;
                break;
            default:
                cout << "This order type is still in development" << endl;
        }
        cout << "Order Status: ";
        switch(OrderStatus){
            case 0: cout << "Filled" << endl;
                break;
            case 1: cout << "Pending" << endl;
                break;
            case 2: cout << "Canceled" << endl;
                break;
            case 3: cout << "Aborted" << endl;
                break;
        }
        cout << "Ticker: " << Ticker << endl;
        cout << "Order Term: ";
        switch(OrderTerm){
            case 0: cout << "AON" << endl;
                break;
            case 1: cout << "GTC" << endl;
                break;
            case 2: cout << "IOC" << endl;
                break;
            case 3: cout << "FOK" << endl;
                break;
            case 4: cout << "Day" << endl;
                break;
        }
        cout << "--------------------------------\n " << endl;
    }
};

#endif /* Header_h */
