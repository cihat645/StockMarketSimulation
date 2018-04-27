//  main.cpp
//  StockMarket
//
//  Created by Thomas Ciha on 3/18/18.
//  Copyright  2018 Thomas Ciha. All rights reserved.
//

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
#include "C:\Users\thomasciha\Oracle\SQLAPI\include\SQLAPI.h"
#include<cstring>
#include<cstdlib>

using namespace chrono;
using namespace std;

SAConnection con;

using namespace std;

struct Account{
    long AccountNo, Balance;
    string UserID, Accounttype,AccountStatus, AccountUserName,AccountPassword, ActivityStatus;
    Account(){
        AccountNo = Balance = -1;
        UserID = ActivityStatus = Accounttype = AccountUserName = AccountPassword = AccountStatus = "";
    }
};

struct ContainsStock{
    long AccountNo,Quantity;
    double PurchasePrice;
    string AssetClass;
    char Ticker[10];
};
struct Stocktable{
    double Price;
    char  AssetClass[15];
    string Ticker;
};

struct optn{
    double price,strike;
    SADateTime EXPDate;
    char AssetClass[15],Optiontype[6];
    char Ticker[10];
};

struct ContainsOption{
    long AccountNo,Quantity;
    double PurchasePrice,Strike;
    char AssetClass[15],OptionType[6];
    char Ticker[10];
    SADateTime ExpDate;
};

// this will show all stocks you hold currently, the price you purchased them at, what price they are at now, the unrealized dollar capital gain, the unrealized
// percentage capital gain. Then the same for all short positions
void ShowPortfolioPositions(long AccNum){
    bool owns_stocks = false;
    int count = 0;
    SACommand stock_ownership(&con,
        " SELECT Ticker, Quantity, PurchasePrice, Price, PurchasePrice * Quantity AS OG_Value, Price * Quantity AS Current_Value, Price * Quantity - PurchasePrice * Quantity AS Unrealized_Cap_Gain_Dollar, (Price * Quantity - PurchasePrice * Quantity)/(PurchasePrice * Quantity) AS Unrealized_Cap_Gain_Percent "
        " FROM StockTable JOIN ContainsStock USING(Ticker) "
        " WHERE AccountNo = :1 "
    );

    stock_ownership << AccNum;
    stock_ownership.Execute();

    while(stock_ownership.FetchNext()){
        if(count == 0){
            cout << "Ticker,  Quantity,   PurchasePrice,  Price,   OG_Value,   Current Value,     Unrealized Capital Gain ($),   Unrealized Capital Gain (%) " << endl;
            owns_stocks = true;
            count++;
        }

        string ticker = (string) stock_ownership.Field("Ticker").asString();
        long quantity = (long) stock_ownership.Field("Quantity");
        double purchase_p = (double) stock_ownership.Field("PurchasePrice").asDouble();
        double price = (double) stock_ownership.Field("Price").asDouble();
        double og_val = (double) stock_ownership.Field("OG_Value").asDouble();
        double curr_val = (double) stock_ownership.Field("Current_Value").asDouble();
        double dollar_cap_g = (double) stock_ownership.Field("Unrealized_Cap_Gain_Dollar").asDouble();
        double perc_cap_g = (double) stock_ownership.Field("Unrealized_Cap_Gain_Percent").asDouble();
        int width = 23;
        cout << ticker << setw(10) << quantity << setw(14) << purchase_p << setw(14) << price << setw(14) << og_val << setw(14) << curr_val << setw(width) << dollar_cap_g << setw(width) << perc_cap_g * 100 << endl;
    }
    if(!owns_stocks)
        cout << "This account currently has no stock holdings" << endl;
}

long login() {
char ans;
Account MyAccount;

        string temp;
        cout<<"Enter your Account UserName:  ";
        getline(cin,temp);
        MyAccount.AccountUserName = temp;
        cout<<"Now Enter your Password:  ";
        getline(cin, temp);
        MyAccount.AccountPassword = temp;

    SACommand get_account_no(&con,
              "SELECT AccountNo "
              "FROM Account "
              "WHERE AccountUserName = :1 "
              "AND AccountPassword = :2 ");

    get_account_no << MyAccount.AccountUserName.c_str() << MyAccount.AccountPassword.c_str();
    get_account_no.Execute();

        if(get_account_no.FetchNext()){
            MyAccount.AccountNo = (long) get_account_no.Field("AccountNo");
            cout<<"Is your Account Number  " << MyAccount.AccountNo<<" (y/n)? ";
            cin>>ans;
            if(ans=='y'||ans == 'Y')
                cout << "Login Successful!" << endl;
            return MyAccount.AccountNo;
        }

    else {
        cout << "failed login attempt"<< endl;
        return 0;
    }
      return MyAccount.AccountNo;
}

void ShowAccountInfo(long accountNo){
    Account Account;
    Stocktable S;
    ContainsStock C;
    int OrderPrice;
    double DollarChange;
    double PercentChange;

SACommand cmdAccountInfo(&con,
                        "SELECT AccountNo,Ticker,Quantity,Balance,ActivityStatus,PurchasePrice FROM Account Natural Join Stocktable Natural Join containsstock WHERE AccountNo = :1");

    cmdAccountInfo<<accountNo;
    cmdAccountInfo.Execute();

    if(!cmdAccountInfo.FetchNext()){
        cout<<"Account Has No previous Activity.\n\n";
    }
    else{
        cout<<"Your Account Information: \n\n\n";
        cout<<"  AccountNo  Balance   Ticker    Quantity  OrderPrice   ActivityStatus    DollarChange     PercentChange\n";
        cout<<" ----------  -------   -----     -------   ------------    -----------    -------------    -------------\n";
        do{
            Account.AccountNo = (long) cmdAccountInfo.Field("AccountNo");
            Account.Balance = (long) cmdAccountInfo.Field("Balance");
            S.Ticker =  (string) cmdAccountInfo.Field("Ticker").asString();
            C.Quantity = (long) cmdAccountInfo.Field("Quantity");
            C.PurchasePrice= (double) cmdAccountInfo.Field("PurchasePrice");
            Account.ActivityStatus = (string)cmdAccountInfo.Field("ActivityStatus").asString();
            DollarChange = S.Price - C.PurchasePrice;
            PercentChange = (S.Price - C.PurchasePrice)/C.PurchasePrice;
            OrderPrice = C.Quantity * C.PurchasePrice;
            
            cout<<Account.AccountNo << "        "   << Account.Balance << "          "
            << S.Ticker << "       "   << C.Quantity << "          " << OrderPrice << "         "
           << Account.ActivityStatus << "           " << DollarChange << "      " << PercentChange << endl;

        } while(cmdAccountInfo.FetchNext());
    }
}

//=================================== NECESSRY FUNCTIONS==========================================
milliseconds convert_to_milliseconds(char *t){
    // selecting substrings of timestamp
    string hrs, mins, secs, ms, timestamp = string(t);
    hrs = timestamp.substr(0,2); // (pos, len)
    mins = timestamp.substr(3,2);
    secs = timestamp.substr(6,2);
    ms = timestamp.substr(9,3);

    //converting substrings to respective chrono types
    hours chrs (stoi(hrs));
    minutes c_mins (stoi(mins));
    seconds c_secs (stoi(secs));
    milliseconds c_ms (stoi(ms));
    milliseconds millisec = duration_cast<milliseconds> (chrs) + duration_cast<milliseconds>(c_mins) + duration_cast<milliseconds>(c_secs) + c_ms;
    return millisec;
}

Offer create_offer_with_dest(string offer){ //creates an offer object subsequent to parsing the data
    stringstream ss(offer);
    char *temp = new char[80];
    Type offer_type;
    float offer_price;
    int qty;

    ss.getline(temp, 50, ',');
    milliseconds time = convert_to_milliseconds(temp);

    ss.getline(temp, 50, ','); //parsing offer type
    if(strcmp(temp,"QUOTE BID") == 0 || strcmp(temp,"QUOTE BID NB") == 0)
        offer_type = Bid;
    else if(strcmp(temp, "QUOTE ASK") == 0 || strcmp(temp, "QUOTE ASK NB") == 0)
        offer_type = Ask;
    else
        return Offer(-1,-1,Bid,time); //ignoring "Trade" and "Trade NB" event types

    ss.getline(temp, 50, ','); // parsing ticker, but not using it rn
    ss.getline(temp, 50, ','); // parsing price
    offer_price = atof(temp);
    ss.getline(temp, 50, ','); // parsing qty
    qty = atoi(temp);
    ss.getline(temp, 50, ','); // parsing exchange

    //there is one more substring in ss, but we aren't using them
    return Offer(offer_price,qty,offer_type,time, temp);
}

void Print_ECN_Stats(vector<class ECN *> All_ECNs){
    cout << "\n\n----------- ECN STATS -----------  " << endl;
    for(int i = 0; i < All_ECNs.size(); i++){
        cout << All_ECNs[i]->ECN_Name << " Data: " << endl;
        cout << "current bids size: ";
        cout << All_ECNs[i]->market.current_bids.size() << endl;
        cout << "current asks size: ";
        cout << All_ECNs[i]->market.current_asks.size() << endl;
        cout << "number of transactions processed: ";
        cout << All_ECNs[i]->num_of_transactions;
        cout << "\n------------------\n";
    }

}

// This function is useful in determining what price you should enter to test limit orders. Since we have no visual of the price at the time we would like to submit
// the order, we can use this function to obtain price data relevant to the time of the limit order we desire to place. The order should be filled if the limit price
// is contained within this interval. This function could also be changed to return the time of the high, the time of the low and bth the high and low prices. With this
// modification we could use this function to generate thousands of test orders.

// m - the number of milliseconds past midnight that corresponds to the time of the beginning of the interval
void get_price_range(milliseconds m, milliseconds fifteen_mins){
    ifstream read_data("C:/Users/thomasciha/DatabaseProject/DatabaseInterface/IBM.csv");
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


// ============================= Route Offer ================================================
//this function assumes the order of ECN insertion is: NASDAQ("NASDAQ"), NASDAQ_PSX("NASDAQ PSX"), CSE("CSE"), NYSE("NYSE"), EDGA("EDGA"), NASDAQ_BX("NASDAQ BX"), FINRA("FINRA"), EDGX("EDGX"), ARCA("ARCA"), BATS("BATS"), BATS_Y("BATS Y")

vector<order_info> route_offer(Offer &o, vector<class ECN *> &all_ecns){ //routes offers parsed from the data file to their respective ECN
    vector<order_info> transaction_data;
    return (o.exchange == "NASDAQ") ? transaction_data = all_ecns[0]->ParseOffer_TEST(o) :
    (o.exchange == "NASDAQ PSX") ? transaction_data = all_ecns[1]->ParseOffer_TEST(o) :
    (o.exchange == "CSE") ? transaction_data = all_ecns[2]->ParseOffer_TEST(o) :
    (o.exchange == "NYSE") ? transaction_data = all_ecns[3]->ParseOffer_TEST(o) :
    (o.exchange == "EDGA") ? transaction_data = all_ecns[4]->ParseOffer_TEST(o) :
    (o.exchange == "NASDAQ BX") ? transaction_data = all_ecns[5]->ParseOffer_TEST(o) :
    (o.exchange == "FINRA") ? transaction_data = all_ecns[6]->ParseOffer_TEST(o) :
    (o.exchange == "EDGX") ? transaction_data = all_ecns[7]->ParseOffer_TEST(o) :
    (o.exchange == "ARCA") ? transaction_data = all_ecns[8]->ParseOffer_TEST(o) :
    (o.exchange == "BATS") ? transaction_data = all_ecns[9]->ParseOffer_TEST(o) : transaction_data = all_ecns[10]->ParseOffer_TEST(o);
}


int UserOptions(){ // after logging in successfully, we will prompt the user for what they want to do
    int selection = 0;
    do{
        cout << "Select from the following menu: (enter 0 to quit) " << endl;
        cout << "1. View account information" << endl;
        cout << "2. View portfolio " << endl;
        cout << "3. Place an order in simulation" << endl;
        cout << "4. Deposit Cash" << endl;
        cin >> selection;
    }
    while(selection < 0 || selection > 4);

    return selection;
}


void deposit_cash(long AccNum){
    double deposit, curr_bal;
    cout << "Enter the amount of cash you are depositing: ";
    cin >> deposit;

    SACommand get_cash(&con,
        " SELECT Balance "
        " FROM Account "
        " WHERE AccountNo = :1"
    );

    SACommand update_cash(&con,
        "UPDATE Account "
        "SET Balance = :1 "
        "WHERE AccountNo = :2 "
     );
    get_cash << AccNum;
    get_cash.Execute();
    if(get_cash.FetchNext()){
        curr_bal = (double) get_cash.Field("Balance").asDouble();
        update_cash << (deposit + curr_bal) << AccNum;
        update_cash.Execute();
    }
    else
        cout << "Account does not exist " << endl;
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
    //earliest time to submit an order is 4AM, latest time (for this system) is 5PM. 5PM is 17 hours after midnight.
    for(int i = 4; i <= 17; i++){
        hours temp(i);
        all_hrs.push_back(duration_cast<milliseconds>(temp));
    }
    for(int i = 0; i < 60; i++){
        minutes temp(i);
        all_mins.push_back(duration_cast<milliseconds>(temp));
    }
    return all_hrs[hr - 4] + all_mins[m];
}

Order create_custom_order(long AccNum){
    string action, term, type;
    float price;
    int qty;
    long order_number;
    milliseconds time;
    OrdType o_type;
    Action o_action;
    Term o_term;

    cout << "CREATION OF CUSTOM STOCK ORDER FOR IBM: " << endl;
    //get order action
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

    //get order size
    do {
        cout << "Enter order size: ";
        cin >> qty;
    } while(qty < 0);

    // get order type
    do{
        cout << "Enter 'Market' or 'Limit' to select the order type: ";
        cin >> type;
    } while(type != "Market" && type != "Limit");
    (type == "Market") ? o_type = Market : o_type = Limit;


    time = get_custom_timestamp(); // get timestamp

    if(o_type == Limit){
        cout << "Since you're processing a limit order, you may want to consider a 15 min price range of your timestamp to ensure your order will be filled." << endl;
        minutes fifteen(15);
        milliseconds fifteen_mins = duration_cast<milliseconds>(fifteen);
        get_price_range(time, fifteen_mins);
    }


    //get order price
    do{
        cout << "Enter the order price: ";
        cin >> price;
    } while(price < 0);

    //get order number
    SACommand get_max_order_num(&con,
        " SELECT MAX(OrderNum)"
        " FROM StockOrder "
    );
    get_max_order_num.Execute();
    if(get_max_order_num.FetchNext()){
        order_number = (long) get_max_order_num.Field("MAX(OrderNum)").asLong();
        order_number++;
    }
    else
        order_number = 1;

    return Order(order_number,AccNum, qty, Stock, "IBM", time, o_type, o_term, o_action, price); // this should increase cash balance by 100
}

int main(){

    char Username[30],password[30];
    cout<<"Oracle UserName:  ";
    cin.getline(Username,29);
    cout<<" Password:  ";
    cin.getline(password,29);
    try{
        con.Connect(
            "iras.ad.stfx.ca:1521/CSCI275",
            Username,password,SA_Oracle_Client);

    cout<<"Connected to Oracle\n";

    long account_num = login();
    if(account_num == 0)
        return 0;
// ======================== =========== =====================================================================
    bool  app_running = true;
do {
    bool Simulation = false;
    string again;
    hours hrs(9), eleven(11), ten(10);
    minutes mns(45), mns2(35), mns3(40), fifteen(15);
    milliseconds time(1), nine_forty_five_AM = duration_cast<milliseconds>(hrs) + duration_cast<milliseconds>(mns), nine_thirty_fiveAM = duration_cast<milliseconds>(hrs) + duration_cast<milliseconds>(mns2);
    milliseconds nine_forty_AM = duration_cast<milliseconds>(hrs) + duration_cast<milliseconds>(mns3);
    milliseconds eleven_forty_AM = duration_cast<milliseconds>(eleven) + duration_cast<milliseconds>(mns3);
    milliseconds ten_thirty_fiveAM = duration_cast<milliseconds>(ten) + duration_cast<milliseconds>(mns2);

    //milliseconds cust = get_custom_timestamp();

    //cout << "custom timestamp = " << cust.count() << endl;
    //cout << "9:40 am " << nine_forty_AM.count() << endl;

    int option = UserOptions();
    switch(option){
        case 1: ShowAccountInfo(account_num);
            break;
        case 2: ShowPortfolioPositions(account_num);
            break;
        case 3: Simulation = true;
            break;
        case 4: deposit_cash(account_num);
            break;
        case 0: return 0;
        default:
            cout << "Invalid selection" << endl;
    }

    if(Simulation){
        string again;
    Order simulation_order = create_custom_order(account_num);
    cout << "\nHere is the order you just created: " << endl;
    simulation_order.Print();
// ======================== SIMULATION =====================================================================
    ifstream read_data("/Users/thomasciha/Documents/2nd Year/CS 275/Stock Broker Project/IBM.csv"); // note: there are 1353590 lines
    string line;
    getline(read_data, line); // throw out the header
    class ECN NASDAQ("NASDAQ"), NASDAQ_PSX("NASDAQ PSX"), CSE("CSE"), NYSE("NYSE"), EDGA("EDGA"), NASDAQ_BX("NASDAQ BX"), FINRA("FINRA"), EDGX("EDGX"), ARCA("ARCA"), BATS("BATS"), BATS_Y("BATS Y");

    vector<class ECN *> All_ECNs; // adding every ECN to vector of ECN pointers so we can initialize the OMS
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

    OMS my_broker(All_ECNs); // instantiating OMS

    int order_count = 0; // counts number of offers in data set, ensures we only process the order once
    vector<order_info> order_info_vec, my_order_info; // my_order_info -> used to store information pertaining to user's order

    minutes fifteen(15);
    milliseconds fifteen_mins = duration_cast<milliseconds>(fifteen), time(1);
    cout << "Starting simulation: " << endl;

    while(getline(read_data, line) && time.count() <= (simulation_order.timestamp.count() + fifteen_mins.count())){ // adding 15 mins cause order may take time to be processed
        Offer temp = create_offer_with_dest(line);
        order_info_vec = route_offer(temp, All_ECNs);
                time = temp.timestamp;
                        if(time.count() >= simulation_order.timestamp.count() && order_count < 1){
                            my_order_info = my_broker.ProcessOrder(simulation_order, con, account_num);
                            order_count++;
                        }
        if(order_info_vec.size() > 0){
            cout << "error "<< endl; // not supposed to get any order_info from orders that do not originate from our OMS (i.e. offers from the file)
            break;
        }
    }

    my_broker.DisplayPendingOrders();
    Print_ECN_Stats(All_ECNs);

    read_data.close();
    }

        cout << "\n\nWould you like to return to the main menu? (enter 'y')" << endl;
        getline(cin, again);
        if(again !=  "Y" || again != "y"){
            app_running = true;
        }
  } while(app_running);

    con.Disconnect();

    cout<<"Disconnected from Oracle\n";
}

 catch(SAException &x)
    {
        try
        {
            // on error rollback changes
            con.Rollback();
        }
        catch(SAException &){}
        
        // print error message
        cout << (const char*)x.ErrText() << endl;
    }
}
