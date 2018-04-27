//
//  OrderManagementSystem.cpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 2/16/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#include "OrderManagementSystem.hpp"
#include <iostream>
#include <limits>
#include <stdio.h>
#include "Order.h"
#include "Offer.hpp"
#include "ECN.hpp"

using namespace std;


// ================================================== ABOUT THE OMS ======================================================================

// The OMS must compute the benefits of internalizing the order (if feasible) vs. sending it to an ECN or market maker. This means the OMS needs to contemplate the trade-off between capital gain via internalization vs discovering optimal ECN routing method. It must also do so in compliance with SEC regulations. Market makers (and OTC market makers) incentivize brokers to route orders to them by offering a form of payment, called "Payment for order flow". We will not be incorporating this into the model as of now.

// As stated on the SEC website: In deciding how to execute orders, your broker has a duty to seek the best execution that is reasonably available for its customers' orders. That means your broker must evaluate the orders it receives from all customers in the aggregate and periodically assess which competing markets, market makers, or ECNs offer the most favorable terms of execution.

// When a broker scans the market in search of better execution prices, they are effectively looking for the "opportunity of price improvement". "Price improvement" is the opportunity, but not the guarantee, for an order to be executed at a better price than the current quote. (I'm assuming the current quote is the price of the latest trade).

// This function will process the front-most order in the queue, which consists of converting the order in to an offer and then sending that to either the ECN or internalizing the offer.

/*  Three potential choices for routing:
    1) ECN
    2) Broker's Inventory (internalize order)
    3) Match order with order from another client
 */

// ========================================================================================================================================

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

pair<float, int> OMS::get_internalization_price_and_qty(string tick, SAConnection &con, long AccNum){  // if we have the stock in our inventory, return the avg price

     float price;
     int qty;

     SACommand find_price_and_qty(
        "SELECT Price, Quantity"
        "FROM StockInventory"
        "WHERE Ticker = :1 "
     );

     find_price_and_qty << tick;
     find_price_and_qty.Execute();

     price = (float) find_price_and_qty.Field("Price");
     qty = (int) find_price_and_qty.Field("Quantity");

     if(qty > 0)
        return make_pair(price,qty);
    
    return make_pair(std::numeric_limits<float>::max(), 0);
}

pair<float, int> OMS::get_best_mkt_price(Type &o){ // returns the index of the best ECN and best price available at that ECN
    int best_ecn_index = 0;
    float best_price;
    if(o == Bid){ // looking for lowest ask to buy into
        best_price = std::numeric_limits<float>::max();

        for(int i = 0; i < available_ECNs.size(); i++){
            if(available_ECNs[i]->market.get_best_ask_price() < best_price && !available_ECNs[i]->market.current_asks.empty()){
                best_price = available_ECNs[i]->market.get_best_ask_price();
                best_ecn_index = i;
            }
        }
    }
    else{ // looking for highest bid to sell into
        best_price = -1;

        for(int i = 0; i < available_ECNs.size(); i++){
            if(available_ECNs[i]->market.get_best_bid_price() > best_price && !available_ECNs[i]->market.current_bids.empty()){
                best_price = available_ECNs[i]->market.get_best_bid_price();
                best_ecn_index = i;
            }
        }
    }
    return make_pair(best_price, best_ecn_index);
}

//this function will do all of the checks to ensure the account balance is sufficient, account is not suspended etc.
pair<bool, string> OMS::eligible_order(Order &o, SAConnection &con, long AccNum){
    int num_shares_owned = 0;
    double cash_balance = 0; // num_shares = number of shares owned by the individual (used to verify the eligbility of sell orders)
    string activity_status = "N/A", reason_for_ineligibility = "";
    // QUERY

   //  input: account number, ticker
    // output: balance from account table, accountactivity status from account table and quantity of shares owned for ticker if its a sell order

     SACommand get_eligibility_info_acc(&con,
        " SELECT Balance, ActivityStatus "
        " FROM Account "
        " WHERE AccountNo = :1 "
     );

    SACommand get_eligibility_info_stock(&con,
        " SELECT Quantity "
        " FROM ContainsStock "
        " WHERE Ticker = :1 AND AccountNo = :2 "
     );

     SACommand get_short_info(&con,
        " SELECT Quantity, ShortPrice "
        " FROM ContainsShort "
        " WHERE AccountNo = :1 "
     );
     get_eligibility_info_acc << AccNum;
     get_eligibility_info_acc.Execute();

     if(get_eligibility_info_acc.FetchNext()){
        activity_status = (string) get_eligibility_info_acc.Field("ActivityStatus").asString();
        cash_balance = (long) get_eligibility_info_acc.Field("Balance");
     }
     else
        return make_pair(false, " account does not exist");

    get_eligibility_info_stock << o.getTicker().c_str() << AccNum;
    get_eligibility_info_stock.Execute();
    if(get_eligibility_info_stock.FetchNext()){
        num_shares_owned = (long) get_eligibility_info_stock.Field("Quantity");
    }

     // === ASSESSING COVER ORDER ======
     if(o.getAction() == Cover){
        get_short_info << AccNum;
        get_short_info.Execute();
        if(get_short_info.FetchNext()){ // if the user has a short position
            long short_qty = (long) get_short_info.Field("Quantity");
            if(llabs(short_qty) < o.getSize()) // if attempting to cover more shares in the short position, set order size to num of shares shorted
                o.setSize(llabs(short_qty));
        }
        else
            return make_pair(false, "reason: cannot cover a short position if no short position exists");
     }

     if(activity_status == "Active"){
     Type offer_type;
     (o.getAction() == Buy || o.getAction() == Cover) ? offer_type = Bid : offer_type = Ask;

        if(offer_type == Bid){      // if it's a buy or cover order (which results in an offer of type Bid
            if(o.getOrderType() == Limit){
                if(cash_balance >= commission_rate + (o.getSize() * o.getPrice())){
                        return make_pair(true, "");
                }
                else
                    return make_pair(false, " insufficient account balance");
            }
            else if(o.getOrderType() == Market) {
                if(cash_balance >= commission_rate + (o.getSize() * get_best_mkt_price(offer_type).first))
                    return make_pair(true, "");
                else{
                    return make_pair(false, "insufficient cash balance to fill market 'buy' order given current market conditions");
                }
            }
            else {
                return make_pair(false, "we are not processing orders other than Limit and Market at the moment");
            }
        }

        else { // if its a sell order, does this account own a sufficient number shares? (default setting: if you enter an order to sell X shares for ABC stock where X > (# of ABC shares owned), then you sell all the shares you own of ABC

            if(o.getAction() == Short)
                return make_pair(true, "");
            // Parsing Sell Order
            if(o.getAction() == Sell && num_shares_owned == 0)
                return make_pair(false, "we cannot sell any shares that we do not own, we can only short sell if we own no shares");

            if(num_shares_owned < o.getSize()) // default condition as described above ^
                o.setSize(num_shares_owned);
            }
    }
    else
        return make_pair(false, "Inactive account");

    return make_pair(true,"");
}

// find_pending_order returns an iterator pointing to the order in pending_orders which corresponds to the order_info object's id
vector<Order>::iterator OMS::find_pending_order(int id){
    vector<Order>::iterator order_iter = pending_orders.end();
    for(vector<Order>::iterator it = pending_orders.begin(); it != pending_orders.end(); it++)
        if(it->getOrderNum() == id)
            return it;

    return order_iter; // return a DUD iterator
}

void update_order_table_query(SAConnection &con, Order &o, long AccNum){
        string UserID;
        SACommand get_userID (&con,
            " SELECT UserID "
            " FROM Account "
            " WHERE AccountNo = :1"
        );
        get_userID << AccNum;
        get_userID.Execute();
        if(get_userID.FetchNext())
            UserID = (string) get_userID.Field("UserID").asString();
        else {
            cout << "No userID was obtained. ERROR: update_order_table_query failed" << endl;
            return;
        }
        string order_types[] = {"L", "M", "MOC", "SOQ", "LOC", "HS", "TSD", "TSP"};
        string order_terms[] = {"AON", "GTC", "IOC", "FOK", "DAY"};
        string order_actions[] = {"Buy", "Sell", "Short", "Cover"};
        string order_statuses[] = {"Filled", "Pending", "Canceled", "Aborted"};
        string type = "Stock", order_type = order_types[o.getOrderType()], order_term = order_terms[o.getTerm()];
        string order_action = order_actions[o.getAction()], order_status = order_statuses[o.getStatus()], time = (string) to_string(o.getTime().count());
        SACommand update_order_table(&con,
        "INSERT INTO StockOrder VALUES(:1, :2, :3, :4, :5, :6, :7, :8,:9,:10,:11, :12, :13) "
        );

        update_order_table.Param(1).setAsLong() = o.getOrderNum();
        update_order_table.Param(2).setAsString() = type.c_str();
        update_order_table.Param(3).setAsString() = o.getTicker().c_str();
        update_order_table.Param(4).setAsLong() = AccNum;
        update_order_table.Param(5).setAsString() = UserID.c_str();
        update_order_table.Param(6).setAsString() = order_type.c_str();
        update_order_table.Param(7).setAsString() = order_term.c_str();
        update_order_table.Param(8).setAsString() = order_action.c_str();
        update_order_table.Param(9).setAsString() = time.c_str();
        update_order_table.Param(10).setAsLong() = o.getSize();
        update_order_table.Param(11).setAsString() = order_status.c_str();
        update_order_table.Param(12).setAsDouble() = o.fill_price;
        update_order_table.Param(13).setAsDouble() = o.getPrice();
        update_order_table.Execute();
}


void OMS::Update_OrderTable(Order &o, SAConnection &con, long AccNum){ // this function updates the order table in the DBMS
    // Queries:
    // input: order, AccountNo
    //output: nothing in app, we are writing to DBMS the respective changes

      SACommand get_cash_balance(&con,
        "SELECT Balance "
        "FROM Account "
        "WHERE AccountNo = :1 "
     );

    SACommand update_cash(&con,
        "UPDATE Account "
        "SET Balance = :1 "
        "WHERE AccountNo = :2 "
     );

     SACommand check_contains_stock(&con,
        "SELECT Quantity, PurchasePrice "
        "FROM ContainsStock "
        "WHERE AccountNo = :1 "
        "AND Ticker = :2 "
     );

     SACommand update_contains_stock(&con,
        "UPDATE ContainsStock "
        "SET Quantity = :1, PurchasePrice = :2 "
        "Where Ticker = :3 AND AccountNo = :4 "
     );


     SACommand insert_stock_position(&con,
        "INSERT INTO ContainsStock "
        "VALUES (:1, :2, :3, :4, :5) "
     );


     SACommand remove_stock_position(&con,
        "DELETE FROM ContainsStock "
        "WHERE AccountNo = :1 "
        "AND Ticker = :2 "
     );

     SACommand insert_short_position(&con,
        "INSERT INTO ContainsShort "
        "VALUES (:1, :2, :3, :4, :5) "
     );

     SACommand check_contains_short(&con,
        "SELECT Quantity, ShortPrice "
        "FROM ContainsShort "
        "WHERE AccountNo = :1 "
        "AND Ticker = :2 "
     );

     SACommand update_contains_short(&con,
        "UPDATE ContainsShort "
        "SET Quantity = :1, ShortPrice = :2 "
     );

     SACommand remove_short_position(&con,
        "DELETE FROM ContainsShort "
        "WHERE AccountNo = :1 "
        "AND Ticker = :2 "
     );

    //===== UPDATE CASH BALANCE ======
    double new_cash_bal, og_cash_balance;

    //get original cash_balance
    get_cash_balance << AccNum;
    get_cash_balance.Execute();
    if(get_cash_balance.FetchNext())
        og_cash_balance = (double) get_cash_balance.Field("Balance");
    else
        cout << "No entry for cash balance on this account" << endl;


    // don't need to fetch next() here as we have already validated the account num in the account table upon the user logging in
    if(o.getAction() == Buy || o.getAction() == Cover){
        new_cash_bal = og_cash_balance - (o.fill_price * o.Size) - commission_rate; // deplete cash balance to appropriate level
    }
    else { //replenish cash
        new_cash_bal = og_cash_balance + (o.fill_price * o.Size) - commission_rate;
    }
    update_cash << new_cash_bal << AccNum;        //update cash with query
    update_cash.Execute();
    cout << "OMS: Cash has been updated" << endl;

    //===== UPDATE ORDER TABLE ======
    if(o.getOrderType() == Market && (o.getAction() == Buy || o.getAction() == Cover)) // if we changed o.Price to MAX FLOAT for processing purposes,
        o.setPrice(NULL); // entered price cannot be stored as MAX FLOAT in DBMS and for market, change back to NULL
    update_order_table_query(con, o, AccNum);
    cout << "OMS: Order table has been updated" << endl;

    //===== UPDATE CONTAINS STOCK OR CONTAINS SHORT ======
    long num_shares = 0, new_qty = 0;
    double avg_price = 0, new_avg_price = 0;
    bool already_own_some = false; // flags that a user already owns some of the stock

  if(o.getAction() == Buy || o.getAction() == Sell) {
    cout << "Checking contains stock " << endl; // we are updating ContainsStock
    check_contains_stock << AccNum << o.getTicker().c_str();
    check_contains_stock.Execute();
     if(check_contains_stock.FetchNext()){ // if the user has a position in ContainsStock, adjust it accordingly
        num_shares = (long) check_contains_stock.Field("Quantity"); // num of shares the account currently owns
        avg_price = (double) check_contains_stock.Field("PurchasePrice"); // avg price of those shares owned
        already_own_some = true;
    }
        if(o.getAction() == Buy){
              new_qty = o.getSize() + num_shares; // we initialized num_shares = 0, so if no position this still works
              new_avg_price = ((avg_price * num_shares) + (o.fill_price * o.getSize())) / new_qty;
          }
            else{ //sell order adjustment, say we own 10 shares w/ avg price of 50, we sell 5 shares for avg price of 40.
                  new_qty = num_shares - o.getSize();
                  new_avg_price = avg_price;
            }
            if(new_qty != 0){
               if(already_own_some){
                update_contains_stock << new_qty << new_avg_price << o.getTicker().c_str() << AccNum;
                update_contains_stock.Execute();
               }
                else {
                    string asset_class = "Stock";
                    insert_stock_position << AccNum << o.getTicker().c_str() << (double) o.fill_price << (long) o.Size << asset_class.c_str();
                    insert_stock_position.Execute();
                }
            }
            else { //remove position from contains stock
                remove_stock_position << AccNum << o.getTicker().c_str();
                remove_stock_position.Execute();
                if(o.OrderAction != Sell)
                    cout << "ERROR: removing stock position when order action != SELL";
            }
            cout << "OMS: Contains stock has been updated..." << endl;
  }
    else{ // we are updating ContainsShort
        cout << "OMS: Analyzing portfolio assets... " << endl;
        check_contains_short << AccNum << o.getTicker().c_str();
        check_contains_short.Execute();
        if(check_contains_short.FetchNext()){
            num_shares = (long) check_contains_short.Field("Quantity"); // num of shares the account currently has shorted
            avg_price = (double) check_contains_short.Field("ShortPrice"); // avg price of those shares shorted
            already_own_some = true; // in this case we 'own' a short position
        }
        bool debug = false;
        if(debug){
            cout << "num_shares = " << num_shares << endl;
            cout << "avg price = " << avg_price << endl;
            cout << "already own some = " << already_own_some << endl;
            cout << "o.Size = " << o.Size << endl;
        }

        if(o.OrderAction == Short){ // reduce qty and compute new_avg price
                new_qty = llabs(num_shares - o.Size);
                new_avg_price = llabs((o.fill_price * o.Size) + (avg_price * num_shares) / new_qty);
        }
        else{ // if we bought to cover a short, i.e. o.OrderAction == Cover
            new_qty = num_shares + o.Size;
            new_avg_price = avg_price;
        }

    if(new_qty != 0){
        if(already_own_some){
            cout << "updating containsShort table" << endl;
            update_contains_short << new_qty << new_avg_price;
            update_contains_short.Execute();
        }
        else {
            string asset_class = "Stock";
            long insertion_qty;
            (o.OrderAction == Short) ? insertion_qty = -1 * o.Size : insertion_qty = o.Size;
            insert_short_position << AccNum << o.getTicker().c_str() << (double) o.fill_price << insertion_qty << asset_class.c_str();
            insert_short_position.Execute();
        }
    }
    else { //remove the position from contains short
        remove_short_position << AccNum << o.getTicker().c_str();
        remove_short_position.Execute();
    }
 }
    cout << "\nHERE IS ORDER TABLE UPDATE: " << endl;
    o.Print();
}

/*
 Update_Order_Table() Documentation:
 1.  Why we had to create a new table for short positions.
 -Problem with the way in which we store short positions: if we store short positions in ContainsStock table with negative quantities, then we will need multiple tuples for one account if we want to have a long and a short position concurrently. However, this will not work since (AccountNo, Ticker) = Primary Key

 */


void OMS::update_orders(vector<order_info> &vec, SAConnection &con, long AccNum){
    vector<Order>::iterator order_iter; // iterator points to the order which corresponds to the order_info being examined
    bool debug = true;

    for(vector<order_info>::iterator it = vec.begin(); it != vec.end(); it++){ // might want to make pending_orders a hash table to improve efficiency
        order_iter = find_pending_order(it->order_ID);
        if(order_iter != pending_orders.end()){ // if the order that corresponds to the order_info object is in pending_orders (it should always be)
            if(it->offer_status == Pending){    // if the transaction which returned order_info iterator it did not fill the order completely
                if(it->avg_price > 0){ // see # 1
                    order_iter->fill_price += it->avg_price * it->shares_filled; // we will compute the avg fill price once the order is filled
                }
            }

            else if(it->offer_status == Filled){ // if the transaction which returned order_info iterator it has filled the order
                if(order_iter->fill_price > 0){ // if it was partially filled first, then filled completely
                    order_iter->fill_price = (order_iter->fill_price + (it->avg_price * it->shares_filled)) / order_iter->Size; // compute avg price
                }
                else                                           // was filled completely from the getgo
                    order_iter->fill_price += it->avg_price;

                order_iter->OrderStatus = Filled; // change order status in pending_orders to filled

                if(debug) { //for debugging & verification purposes
                    cout << "\nOMS: erasing order number: " << order_iter->OrderNumber << " from pending orders" << endl;
                    cout << "Before erasing, here's the order_info of that order: " << endl;
                    it->Print();
                }

                Update_OrderTable(*order_iter, con, AccNum); // make appropriate changes to OrderTable in the DBMS
                cout << "ORDER ERASED FROM PENDING ORDERS " << endl;
                pending_orders.erase(order_iter); // eliminate the filled order from pending_orders
            }
        }
        else { // to facilitate in catching errors
            cout << "ERROR: NO MATCHING PENDING ORDER" << endl;
        }
    }
}

/* update_orders Documentation
 1. if order was added but couldn't be filled at all in the ECN, the ECN sets order_info.avg_price = -1. So in this case, there is nothing to be udpated with respect to this order

 */

order_info OMS::internalize_order(Offer &o, pair<float,int> internal_inventory, float i_price, string tick, SAConnection &con, long AccNum){ // called after all checks have been confirmed that we should internalize
    vector<order_info> temp_order_info;

    if(internal_inventory.second >= o.Qty){// if broker can completley fill order
        internal_inventory.second -= o.Qty; // update internal_inventory quantity
        temp_order_info.push_back(order_info(i_price, o.Qty, Filled, o.OrderID, false)); // generate order_info for this order
        internalization_profit += i_price * o.Qty;
    }

    else { //broker can partially fill order using all of our inventory
        temp_order_info.push_back(order_info(i_price, internal_inventory.second, Pending, o.OrderID, false));
        internalization_profit += i_price * internal_inventory.second;
        internal_inventory.second = 0; // updating internal inventory quantity to 0
    }

    //update stock inventory table of DBMS

     SACommand update_stock_inventory(
        "UPDATE StockInventory"
        "SET Quantity = :1"
        "WHERE Ticker = :2"
     );

     SACommand remove_stock_inventory(
        "DELETE FROM StockInventory"
        "WHERE Ticker = :1"
     );
    
    if(internal_inventory.second > 0){ // if stock_inventory has remaining shares
        update_stock_inventory << internal_inventory.second << tick;
        update_stock_inventory.Execute();
    }
    else {
        remove_stock_inventory << tick;
        remove_stock_inventory.Execute();
    }

    // update the order we just internalized
    update_orders(temp_order_info, con, AccNum);
    return temp_order_info[0]; // return order_info for order that was internalized
}

// NOTE: it may be a good idea to return a reason or flag for orders that are denied
vector<order_info> OMS::ProcessOrder(Order &o, SAConnection &con, long AccNum){
    vector<order_info> order_information;
    pair<bool, string> eligibility_info = eligible_order(o,con, AccNum);

     if(eligibility_info.first){
           bool send_to_ecn = true; // flag indicates if we are routing to ecn. default value = true
        // ===== CONVERTING ORDER TO OFFER ========
        if(o.OrderType == Market &&(o.OrderAction == Sell || o.OrderAction == Short))
            o.setPrice(0);                    // no entered price with market orders
        else if(o.OrderType == Market && (o.OrderAction == Buy || o.OrderAction == Cover))
            o.setPrice(std::numeric_limits<float>::max());

        pending_orders.push_back(o);        // add to pending orders
        Offer new_offer(o.getPrice(), o.Size, o.timestamp, o.OrderTerm, o.OrderType, true, o.OrderNumber); // true means the offer was placed from the OMS
        (o.OrderAction == Buy || o.OrderAction == Cover) ? new_offer.OfferType = Bid : new_offer.OfferType = Ask; // setting appropriate offer type

        // ===== DECIDING IF INTERNALIZING IS PROFITABLE & VIABLE ======== (see #1)
        // get internalization price, (it will be MAXFLOAT if we cannot internalize it)
        pair<float, int> internal_inventory = get_internalization_price_and_qty(o.getTicker(), con, AccNum); // see #2

        // get best price in the market and location of that price for the offer generated by the Order
        pair<float, int> best_mkt_price_and_location = get_best_mkt_price(new_offer.OfferType);

        bool internalization_is_profitable = (new_offer.OfferType == Bid) ? (internal_inventory.first < o.getPrice()) : false; // Note: see #3
        bool internalization_is_viable = (new_offer.OfferType == Bid) ? (internal_inventory.first < best_mkt_price_and_location.first) : false; //see #4

        if(internalization_is_viable && internalization_is_profitable){ //we internalize the order
            float internalization_price;
            (o.OrderType == Market) ? internalization_price = best_mkt_price_and_location.first - .01 : internalization_price = o.getPrice();

            order_info internalization_info = internalize_order(new_offer, internal_inventory, internalization_price, o.getTicker(), con, AccNum);
            if(internalization_info.offer_status == Filled)
                send_to_ecn = false; // do not send to ecn if we filled the order
        }

         // SENDING OFFER TO BEST ECN
        else if(send_to_ecn){ // we are not internalizing order
            
            pair<float, int> best_mkt_price_and_location = get_best_mkt_price(new_offer.OfferType);
            cout << "OMS: routing order..." << endl;
            order_information = this->available_ECNs[best_mkt_price_and_location.second]->ParseOffer_TEST(new_offer); //routing order
            cout << "OMS: has retrieved order_info..." << endl;

            // ==== REROUTING INCOMPLETE MARKET ORDER =======
            //remember: order_information.end() points to the order being processed
            int count = 0;
            //cout << "order_information.end incomplete market order = " << order_information.end()->incomplete_market_order << endl;
            while(o.getOrderType() == Market && order_information.end()->incomplete_market_order == true) {
                cout << "Rerouting incomplete market order..." << endl;
                cout << "here is order info before rerouting attempt " << count + 1 << endl;
                if(!order_information.empty()){
                    for(auto i : order_information)
                        i.Print();
                }
                else
                    cout << "No order_info to display" << endl;

                new_offer.Qty -= order_information.end()->shares_filled; //update new_offer qty before rerouting
                best_mkt_price_and_location = get_best_mkt_price(new_offer.OfferType);
                order_information = this->available_ECNs[best_mkt_price_and_location.second]->ParseOffer_TEST(new_offer);
                if(++count > 100){
                    cout << "ERROR: Incomplete market order timed out!" << endl;
                    break;
                }
            }
        //}


        if(order_information.size() > 0)
            update_orders(order_information, con, AccNum);
     }

    else {
        cout << "Order " << o.getOrderNum() << " Ineligible: " << eligibility_info.second << endl;
        o.setStatus(Aborted); // probably need to add this to our order status domain in DB to ensure we don't violate referential integrity constraints
    }
    return order_information;
}


/*
 1. Logic of Internalization:
     Consider the scenario where we own 500 shares of IBM which we obtained for $100 / share. Suppose we want to sell these shares whenever we, the broker, can do so at a profit (for internalization purposes).

     The DBMS table "Stock Inventory" contains the shares of stocks the broker is willing to internalize for a profit as well as the price that was paid to obtain those shares. The OMS takes this information into consideration and will determine if it is both profitable and possible to internalize the order.

     Situation: An incoming buy (limit) order is being processed with a qty of 1000 shares and a price of $110 share. The best market price at the moment is $110.05.

     - this will also internalize offers which have a buy (market) order where the price paid (in inventory) is less than the market price. Then we will fill them at $0.01 below the current best price in the market.

     The OMS will assess the situation as follows:
     - we are willing to sell 500 of our shares to the client who has placed this order because we will earn a profit of 500 * 110 = $55000 AND this provides a better price for those 500 shares than what is available currently in the market. (If this was a market order, the OMS will buy them for $110.04 because we are no longer bound to the constraint of providing a price better than or equal to the price specified by a limit order)

     - Given this, the OMS will fill 500 shares of the order at $110/share using the shares in the broker_inventory. Then it will reduce the quantity of shares in the broker inventory via a SQL query.

     - After filling all or part of the order via internalization, we will need to update_pending_orders() given the order_info that results from this internalization

     - The OMS then checks if any shares are remaining in the order (there are 500 remaining in this case) and will route the order to an ECN for fulfillment.

     - After the order has been filled at the ECN w/ best market price, we will update the pending_orders()

 2. internal_inventory.first = price paid for security by my_broker (obtained from DBMS)
    internal_inventory.second = qty of inventory the broker is willing to internalize (which is obtained from DBMS as well)

 3. Note: right now, we are only internalizing incoming Bids by comparing them to our inventory. In order to determine if we can internalize an incoming Ask we will need to assess all other orders to be processed by the OMS and determine if it is advantageous to buy the shares off the client. Other methods of determining whether to internalize an incoming sell order are beyond the scope of this project.

 4. This function answers the question: is internalizing the order viable given the responsibility of providing the best possible price? It is incumbent on the stock broker to provide the best possible price for a security given current market conditions, so we can only internalize if we meet these constraints.

 */








