
# StockMarketSimulation

Two other students and I collaborated to create a stock broker application system which allows users to:
1.	Place orders in a stock market simulation for IBM common stock
2.	Deposit cash into their brokerage account
3.	View their account information
4.	View their portfolio 

We developed the system in C++ and integrated an Oracle DBMS using SQL queries. I will be explaining the stock market simulation segment of our project as I was the lead backend developer.

Simulation Overview:
I have obtained millisecond market data for IBM stock which includes the details of every bid and ask processed throughout the entire stock market in a single day. These data include:
-	the exchange routing destination
-	quantity of shares
-	entered price 
-	millisecond timestamp
-	the event type 
- order conditions

The simulation begins by prompting the user to create an order for IBM stock. The user must also specify the time of day the order would be submitted if placed in real-time. Once the order has been created, we begin parsing the order data for IBM.

![alt text](https://docs.google.com/drawings/d/e/2PACX-1vQTaNjMCIwPmrbIIGwH_kmUlrmQqLylqtAq5kNwHtymSt7tT4VlvITI-i1gKxapFN3RwxX07yPwXGQy/pub?w=480&h=360)

Once the order has been created, the application converts each line of the market data into an Offer, using the chrono library to typecast the string timestamp into the Offer’s millisecond timestamp. The Offer is then submitted to its corresponding ECN before parsing the next line. Each ECN (Electronic Communication Network) will autonomously match buyers and sellers as each line is streamed, parsed into an Offer and then routed. The transaction crietria underpinning the ECN objects is outlined in further detail in the wiki.

The application continues to submit orders from the file to their respective ECN until we stream the timestamp from the file which corresponds to the timestamp of the user-created order. The user's order is then submitted to to the OMS. The OMS processes the order while taking the ‘current’ market conditions and internalization opportunities into consideration. Once the order has been filled, the client’s account is updated and they will be able to see the average price they have obtained for their order (assuming the order would have been filled).

This is a superficial description of the simulation. Reference the project wiki to delve into the minute details of each major component.

Note: If you wish to run this application locally, you will have to delete the functions containing SQL queries to our database as you will not have access to our information.
