//  main.cpp
//  StockMarket
//
//  Created by Thomas Ciha on 8/18/18.
//  Copyright  2018 Thomas Ciha. All rights reserved.
//
#include <vector>
#include <string>
#include <iostream>
#include <queue>
#include <algorithm>
#include <iostream>
#include <vector>
#include <time.h>
#include <utility>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include "Offer.hpp"
#include "ECN.hpp"
#include "OrderManagementSystem.hpp"
#include <cstring>
#include  <cstdlib>
#include "Storage.hpp"

string FOLDER_PATH = "/Users/thomasciha/Documents/2nd Year/CS 275/StockMarketSimulationLocal/StockMarketSimulationLocal/";
string FILE_PATH = "/Users/thomasciha/Documents/2nd Year/CS 275/StockMarketSimulationLocal/IBM.csv";

//string FOLDER_PATH = "";
//string FILE_PATH = "";                                                // file path for reading market data

using namespace chrono;
using namespace std;

milliseconds convert_to_milliseconds(char *t){
    string hrs, mins, secs, ms, timestamp = string(t);                                                              // selecting substrings of timestamp
    hrs = timestamp.substr(0,2);                                                                                    // (pos, len)
    mins = timestamp.substr(3,2);
    secs = timestamp.substr(6,2);
    ms = timestamp.substr(9,3);

    hours chrs (stoi(hrs));                                                                                         //converting substrings to respective chrono types
    minutes c_mins (stoi(mins));
    seconds c_secs (stoi(secs));
    milliseconds c_ms (stoi(ms));
    milliseconds millisec = duration_cast<milliseconds> (chrs) + duration_cast<milliseconds>(c_mins) + duration_cast<milliseconds>(c_secs) + c_ms;
    return millisec;
}

Offer create_offer_with_dest(string offer){                                                                         // creates an offer object subsequent to parsing the data
    stringstream ss(offer);
    char *temp = new char[80];
    Type offer_type;
    float offer_price;
    int qty;

    ss.getline(temp, 50, ',');
    milliseconds time = convert_to_milliseconds(temp);

    ss.getline(temp, 50, ',');                                                                                      // parsing offer type
    if(strcmp(temp,"QUOTE BID") == 0 || strcmp(temp,"QUOTE BID NB") == 0)
        offer_type = Bid;
    else if(strcmp(temp, "QUOTE ASK") == 0 || strcmp(temp, "QUOTE ASK NB") == 0)
        offer_type = Ask;
    else
        return Offer(-1,-1,Bid,time);                                                                               // ignoring "Trade" and "Trade NB" event types

    ss.getline(temp, 50, ',');                                                                                      // parsing ticker, but not using it rn
    ss.getline(temp, 50, ',');                                                                                      // parsing price
    offer_price = atof(temp);
    ss.getline(temp, 50, ',');                                                                                      // parsing qty
    qty = atoi(temp);
    ss.getline(temp, 50, ',');                                                                                      // parsing exchange

    return Offer(offer_price, qty, offer_type, time, temp);                                                         //there is one more substring in ss, but we aren't using them
}

void Print_ECN_Stats(vector<class ECN *> All_ECNs){
    cout << "\n\n----------- ECN STATS -----------  " << endl;
    for(int i = 0; i < All_ECNs.size(); i++){
        cout << All_ECNs[i]->get_name() << " Data: " << endl;
        cout << "current bids size: ";
        cout << All_ECNs[i]->market.get_current_bids()->size() << endl;
        cout << "current asks size: ";
        cout << All_ECNs[i]->market.get_current_asks()->size() << endl;
        cout << "number of transactions processed: ";
        cout << All_ECNs[i]->get_transactions();
        cout << "\n------------------\n";
    }

}

void get_price_range(milliseconds m, milliseconds fifteen_mins){
    /*
     m - the number of milliseconds past midnight that corresponds to the time of the beginning of the interval

     This function is useful in determining what price you should enter to test limit orders. Since we have no visual of the price at the time we would like to submit
     the order, we can use this function to obtain price data relevant to the time of the limit order we desire to place. The order should be filled if the limit price
     is contained within this interval. This function could also be changed to return the time of the high, the time of the low and bth the high and low prices. With this
     modification we could use this function to generate thousands of test orders.
     */
    ifstream read_data("/Users/thomasciha/Documents/2nd Year/CS 275/StockMarketSimulationLocal/IBM.csv");
    string line;
    milliseconds time(0);
    getline(read_data, line);
    float high = 0, low = 100000;
    while(getline(read_data, line) && time.count() < m.count() + fifteen_mins.count()){
        Offer temp = create_offer_with_dest(line);
        time = temp.timestamp;
        if(time.count() >= m.count()){
            if(high < temp.Price)
                high = temp.Price;
            if(low > temp.Price && temp.Price > 0)
                low = temp.Price;
        }
    }

    cout << "For time interval: " << m.count() << " + 15 minutes" << endl;
    cout << "High = " << high << endl;
    cout << "Low = " << low << endl;
    read_data.close();
}

int UserOptions(){                                                                                      // after logging in successfully, we will prompt the user for what they want to do
    int selection = 0;
    do{
        cout << "Select from the following menu: (enter 0 to quit) " << endl;
        cout << "1. View account information" << endl;
        cout << "2. View portfolio " << endl;
        cout << "3. Place an order in simulation" << endl;
        cout << "4. Deposit Cash" << endl;
        cout << "5. Display order history" << endl;
        cin >> selection;
        if(selection > 5 or selection < 0)
            cout << "Invalid Selection" << endl;
    }
    while(selection < 0 || selection > 5);

    return selection;
}

milliseconds get_custom_timestamp(){
    int hr, m;
    do{
        cout << "Enter the hour of day to place order: (must be between 4 (4AM) and 17 (5PM))";
        cin >> hr;
        if(hr != 17){
            cout << "Enter the minute of that hour to place order: (must be between 0 and 59)";
            cin >> m;
        }
    } while(hr < 4 || hr > 17 || m < 0 || m > 59);

    vector<milliseconds> all_hrs, all_mins;
    for(int i = 4; i <= 17; i++){                                                                    // earliest time to submit an order is 4AM, latest time (for this system) is 5PM. 5PM is 17 hours after midnight.

        hours temp(i);
        all_hrs.push_back(duration_cast<milliseconds>(temp));
    }
    for(int i = 0; i < 60; i++){
        minutes temp(i);
        all_mins.push_back(duration_cast<milliseconds>(temp));
    }
    return all_hrs[hr - 4] + all_mins[m];
}

Order create_custom_order(long AccNum, double num_orders){
    string action, term, type;
    float price;
    int qty;
    long order_number;
    milliseconds time;
    OrdType o_type;
    Action o_action;
    Term o_term;

    cout << "CREATE A STOCK ORDER FOR IBM: " << endl;
                                                                                                                        // get order action
    do{
        cout << "Choose order action: (first letter capitalized, rest lowercase)\nEnter Buy, Sell, Short or Cover: ";
        cin >> action;
    } while(action != "Buy" && action != "Sell" && action != "Short" && action != "Cover");
    (action == "Buy") ? o_action = Buy :
    (action == "Sell") ? o_action = Sell :
    (action == "Cover") ? o_action = Cover : o_action = Short;
                                                                                                                        // get order term
    do{
        cout << "Enter 'Day' or 'GTC' to select the order term: ";
        cin >> term;
    } while(term != "Day" && term != "GTC");
    (term == "Day") ? o_term = Day : o_term = GTC;
    
    do {                                                                                                                // get order size
        cout << "Enter order size: ";
        cin >> qty;
    } while(qty < 0);

    do{                                                                                                                 // get order type
        cout << "Enter 'Market' or 'Limit' to select the order type: ";
        cin >> type;
    } while(type != "Market" && type != "Limit");
    (type == "Market") ? o_type = Market : o_type = Limit;


    time = get_custom_timestamp();                                                                                      // get timestamp

    if(o_type == Limit){
        cout << "Since you're processing a limit order, you may want to consider a 15 min price range of your timestamp to ensure your order will be filled." << endl;
        minutes fifteen(15);
        milliseconds fifteen_mins = duration_cast<milliseconds>(fifteen);
        get_price_range(time, fifteen_mins);
    }

    do{
        cout << "Enter the order price: ";                                                                              // get order price
        cin >> price;
    } while(price < 0);

    order_number = num_orders + 1;
    return Order(order_number, AccNum, qty, "IBM", time, o_type, o_term, o_action, price);                              // this should increase cash balance by 100
}

vector<order_info> route_offer(Offer &o, vector<class ECN *> &all_ecns){
    // assumes order of ECN insertion is: NASDAQ("NASDAQ"), NASDAQ_PSX("NASDAQ PSX"), CSE("CSE"), NYSE("NYSE"), EDGA("EDGA"), NASDAQ_BX("NASDAQ BX"), FINRA("FINRA"), EDGX("EDGX"), ARCA("ARCA"), BATS("BATS"), BATS_Y("BATS Y")
    // routes offers parsed from the data file to their respective ECN
    vector<order_info> transaction_data;
    return (o.exchange == "NASDAQ") ? transaction_data = all_ecns[0]->ParseOffer(o) :
    (o.exchange == "NASDAQ PSX") ? transaction_data = all_ecns[1]->ParseOffer(o) :
    (o.exchange == "CSE") ? transaction_data = all_ecns[2]->ParseOffer(o) :
    (o.exchange == "NYSE") ? transaction_data = all_ecns[3]->ParseOffer(o) :
    (o.exchange == "EDGA") ? transaction_data = all_ecns[4]->ParseOffer(o) :
    (o.exchange == "NASDAQ BX") ? transaction_data = all_ecns[5]->ParseOffer(o) :
    (o.exchange == "FINRA") ? transaction_data = all_ecns[6]->ParseOffer(o) :
    (o.exchange == "EDGX") ? transaction_data = all_ecns[7]->ParseOffer(o) :
    (o.exchange == "ARCA") ? transaction_data = all_ecns[8]->ParseOffer(o) :
    (o.exchange == "BATS") ? transaction_data = all_ecns[9]->ParseOffer(o) : transaction_data = all_ecns[10]->ParseOffer(o);
}


void run_simulation(Order simulation_order, Account *acc, vector<pair<string, pair<int, double>>> oms_stocks){
    bool order_filled = false;
    ifstream read_data(FILE_PATH);                                                                                  // start reading the 1353590 lines
    string line;
    class ECN NASDAQ("NASDAQ"), NASDAQ_PSX("NASDAQ PSX"), CSE("CSE"), NYSE("NYSE"), EDGA("EDGA"), NASDAQ_BX("NASDAQ BX"), FINRA("FINRA"), EDGX("EDGX"), ARCA("ARCA"), BATS("BATS"), BATS_Y("BATS Y");
    int order_count = 0;                                                                                            // counts number of offers in data set, ensures we only process the order once.
    vector<order_info> order_info_vec, my_order_info;                                   // my_order_info -> used to store information pertaining to user's order
    minutes fifteen(15);
    milliseconds patience = duration_cast<milliseconds>(fifteen), time(1);
    milliseconds stop_time = simulation_order.get_time() + patience;                // timestamp on order + patience (the amount of time we will wait for limit orders to fill in the market before exiting simulation)

    cout << "\nHere is the order we're simulating: " << endl;
    simulation_order.Print();
    getline(read_data, line);                                                                                       // throw out the header
    
    vector<class ECN *> All_ECNs;                                                                                   // adding every ECN to vector of ECN pointers so we can initialize the OMS
    All_ECNs.push_back(&NASDAQ);
    All_ECNs.push_back(&NASDAQ_PSX);
    All_ECNs.push_back(&CSE);
    All_ECNs.push_back(&NYSE);
    All_ECNs.push_back(&EDGA);
    All_ECNs.push_back(&NASDAQ_BX);
    All_ECNs.push_back(&FINRA);
    All_ECNs.push_back(&EDGX);
    All_ECNs.push_back(&ARCA);
    All_ECNs.push_back(&BATS);
    All_ECNs.push_back(&BATS_Y);
    
    OMS my_broker(All_ECNs);                                                            // instantiating OMS
    my_broker.AddAccount(acc);                                                          // adding our sample account to the OMS
    if(oms_stocks.size() > 0){                                                          // if we have stock inventory to add
        vector<pair<string, pair<int, double>>>::iterator it;
        for(it = oms_stocks.begin(); it != oms_stocks.end(); it++)
            my_broker.AddStock(it->first, it->second.first, it->second.second);         // add stock iventory to OMS
    }

    cout << "Starting simulation: " << endl;
    
    while(getline(read_data, line) && time.count() <= stop_time.count()){
        Offer temp = create_offer_with_dest(line);                                      // create offer from current line of data file
        order_info_vec = route_offer(temp, All_ECNs);                                   // route that offer among the exchanges
        time = temp.timestamp;                                                          // update current time from the offer just parsed

        if(order_info_vec.size() > 0){
            cout << "Error: Erroneous Order Info "<< endl;                              // not supposed to get any order_info from orders that do not originate from our OMS (i.e. offers from the file)
            break;
        }
        
        if(time.count() >= simulation_order.get_time().count() && order_count < 1){     // once we reach the time of our order in the market
            cout << "At the time of your order submission, this is the market: ";
            Print_ECN_Stats(All_ECNs); cout << endl;                                    // show current market conditions
            my_order_info = my_broker.ProcessOrder(simulation_order);                   // Send your order to the OMS for processing
            order_count++;
            if(my_order_info.size() > 0){
                if(my_order_info[0].offer_status == Filled)
                        order_filled = true;
            }
        }

        if(time.count() >= stop_time.count() && !order_filled){
            cout << "+-----------------------------------------------------------------------------------------------------------------------------+\n";
            cout << "|  Your order was not filled. This is typically caused by a limit order price that does not meet current market conditions.   |" << endl;   // limit orders wil be discarded if they're not filled within the patience parameter
            cout << "+-----------------------------------------------------------------------------------------------------------------------------+\n";
            break;
        }
    }
    my_broker.DisplayPendingOrders();                                                   // Display any pending orders in the OMS
    read_data.close();
}

void show_account_before(Account &my_account){
    cout << "Portfolio before order: ";
    my_account.display_portfolio();
    cout << "Orders before order: " << endl;
    my_account.display_orders();
    cout << "\n\nAccount Information before order: " << endl;
    my_account.display_account_info();
}

void show_account_after(Account &my_account){
    cout << "\nPortfolio after order: ";
    my_account.display_portfolio();
    cout << "\n\nOrders after order: " << endl;
    my_account.display_orders();
    cout << "\n\nAccount Information after order: " << endl;
    my_account.display_account_info();
}


void run_test_cases(){
    /*
     Runs a bunch of test cases to ensure the system is functioning properly.
     Times will be near market open to decrease run time.
     */
    // ---------------------------- create timestamps ----------------------------
    hours eight(8), nine(9), ten(10),  eleven(11), one(1);
    minutes mns(45), mns2(35), mns3(40), fifteen(15);
    milliseconds time(1), nine_forty_five_AM = duration_cast<milliseconds>(nine) + duration_cast<milliseconds>(mns), nine_thirty_fiveAM = duration_cast<milliseconds>(nine) + duration_cast<milliseconds>(mns2);
    milliseconds nine_forty_AM = duration_cast<milliseconds>(nine) + duration_cast<milliseconds>(mns3);
    milliseconds eleven_forty_AM = duration_cast<milliseconds>(eleven) + duration_cast<milliseconds>(mns3);
    milliseconds ten_thirty_fiveAM = duration_cast<milliseconds>(ten) + duration_cast<milliseconds>(mns2);
    milliseconds one_hour = duration_cast<milliseconds>(one);
    milliseconds fifteen_mins = duration_cast<milliseconds>(fifteen);
    milliseconds eight_AM = duration_cast<milliseconds>(eight), eight_forty_five_AM = duration_cast<milliseconds>(eight) + mns;
    
    // ---------------------------- create Account ----------------------------
    class Account my_account(1);                                                                                                // create a sample account
    my_account.set_balance(100000);
    my_account.add_long_position("IBM", 100, 100.36);
    my_account.add_long_position("AAPL", 50, 93.43);                                                                            // adding positions to account. You can toggle these to how the system will respond to the orders below
    my_account.add_short_position("IBM", 70, 175);

    
    // ---------------------------- create Orders ----------------------------
    // Order(OrderNumber, AccountNumber, Size, Ticker, timestamp, OrderType, OrderTerm, OrderAction, Price, fill_price)         // fill price is entered by OMS afterwards and is initialized to 0 in the Order constructor
    Order sample_mkt_buy = Order(1, my_account.get_account_num(), 19, "IBM", nine_forty_AM, Market, Day, Buy, 0);
    Order sample_limit_buy = Order(2, my_account.get_account_num(), 75, "IBM", nine_forty_five_AM, Limit, Day, Buy, 155);       // used "get_price_range(nine_thirty_fiveAM, fifteen_mins);" to come up with a limit price
    Order sample_limit_buy2 = Order(3, my_account.get_account_num(), 5, "IBM", nine_forty_AM, Limit, Day, Buy, 120);            // limit order with too low of a price to get filled
    Order sample_mkt_sell = Order(4, my_account.get_account_num(), 7, "IBM", nine_thirty_fiveAM, Market, Day, Sell, 0);
    Order sample_limit_sell = Order(5, my_account.get_account_num(), 15, "IBM", ten_thirty_fiveAM, Limit, Day, Sell, 154);
    Order sample_short_mkt_sell = Order(6, my_account.get_account_num(), 15, "IBM", nine_thirty_fiveAM, Market, Day, Short, 0);
    Order sample_short_limit_sell = Order(7, my_account.get_account_num(), 19, "IBM", nine_forty_AM, Limit, Day, Short, 152);
    Order sample_mkt_cover = Order(8, my_account.get_account_num(), 95, "IBM", eleven_forty_AM, Market, Day, Cover, 0);
    Order sample_limit_cover = Order(9, my_account.get_account_num(), 10, "IBM", ten_thirty_fiveAM, Limit, Day, Cover, 154);
    Order early_mkt_buy = Order(10, my_account.get_account_num(), 65, "IBM", eight_forty_five_AM, Market, Day, Buy, 0);
    Order limit_buy_int = Order(11, my_account.get_account_num(), 10, "IBM", nine_forty_AM, Limit, Day, Buy, 170);
    
    vector<pair<string, pair<int, double>>> stock_inventory;
    
    // ---------------------------- Simulate Orders ----------------------------
//    cout << "-------------------------------------------- Sample Market Buy --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_mkt_buy, &my_account, stock_inventory);
//    show_account_after(my_account);

    
//    cout << "-------------------------------------------- Limit Buy --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_limit_buy, &my_account, stock_inventory);
//    show_account_after(my_account);
//
//    cout << "-------------------------------------------- Limit Buy 2 --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_limit_buy2, &my_account, stock_inventory);
//    show_account_after(my_account);
    
//    cout << "-------------------------------------------- Market Sell --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_mkt_sell, &my_account, stock_inventory);
//    show_account_after(my_account);
    
//    cout << "-------------------------------------------- Limit Sell --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_limit_sell, &my_account, stock_inventory);
//    show_account_after(my_account);
    
//    cout << "-------------------------------------------- Short Market Sell --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_short_limit_sell, &my_account, stock_inventory);
//    show_account_after(my_account);
    
//    cout << "-------------------------------------------- Market Cover --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_mkt_cover, &my_account, stock_inventory);
//    show_account_after(my_account);
    
//    cout << "-------------------------------------------- Limit Cover --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(sample_limit_cover, &my_account, stock_inventory);
//    show_account_after(my_account);
    
//    cout << "-------------------------------------------- Market Buy w/little Liquidity --------------------------------------------" << endl;
//    show_account_before(my_account);
//    run_simulation(early_mkt_buy, &my_account, stock_inventory);
//    show_account_after(my_account);
    
//    cout << "-------------------------------------------- Limit Buy Internalized --------------------------------------------" << endl;    // create scenario where it makes sense to internalize
//    stock_inventory.push_back(make_pair("IBM", make_pair(15, 140.24)));
//    get_price_range(nine_forty_AM, fifteen_mins);
//    show_account_before(my_account);
//    run_simulation(limit_buy_int, &my_account, stock_inventory);
//    show_account_after(my_account);
    
    //    cout << "-------------------------------------------- Limit Buy Not Internalized --------------------------------------------" << endl;    // create scenario where it doesn't make sense to interanalize
//    vector<pair<string, pair<int, double>>> stock_inventory2;
//    stock_inventory2.push_back(make_pair("IBM", make_pair(40, 200)));
//    show_account_before(my_account);
//    run_simulation(limit_buy_int, &my_account, stock_inventory2);
//    show_account_after(my_account);
}



int main(){
    
//    run_test_cases();
    
    bool app_running = true;
    Account my_account(999);
    my_account.set_balance(100000);
    
    do {
        int option = UserOptions();
        switch(option){
            case 1: my_account.display_account_info();
                break;
            case 2: my_account.display_portfolio();
                break;
            case 3: {
                Order custom_order = create_custom_order(my_account.get_account_num(), my_account.get_num_orders());
                vector<pair<string, pair<int, double>>> Inventory;
                run_simulation(custom_order, &my_account, Inventory);
                break;
            }
            case 4: my_account.deposit_cash();
                break;
            case 5: my_account.display_orders();
                break;
            case 0: app_running = false;
                break;
            default:
                cout << "Invalid selection" << endl;
        }

    } while(app_running);
}



/*
 FEATURES THAT CAN BE ADDED:
  - if an order was filled at more than one stock exchange, show the composition of shares that were filled at each exchange
  - add additional order types
  - can add short sell checking. This simulation assumes that the broker will always have shares available to lend to clients for short selling. In reality, brokers will only use a subset of their assets to lend to clients for shorting.
  - can add more stocks than just IBM. Free time series data available via IEX API, or other data vendors.
  - add capital gain / loss functionality
  - add level 2 functionality (seeing best bids and asks stacked by volumes and prices)
 */
