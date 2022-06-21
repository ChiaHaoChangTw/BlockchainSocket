# Socket Programming for Blockchain Applications
There are 2 Clients, 1 Main Server, and 3 Transaction Servers in this project. Clients communicate with the Main Server via TCP, and Mian Server communicates with the Transaction Servers via UDP. 4 kinds of operations can be carried out by clients - CHECK WALLET, TXCOINS, TXLIST, and STATS.

### CHECK WALLET
In this operation, Main Server contacts 3 Transaction Servers to gather the information of the requested username, and return the calculated amount in username's wallet.

Command for this operation:
~~~
./[clientname] [username]
~~~

### TXCOINS
In this operation, Main Server contacts 3 Transaction Servers to complete and record the transaction in one of the Transaction Servers.

Command for this operation:
~~~
./[clientname] [sender username] [receiver username] [transfer amount]
~~~

### TXLIST
Client sends request to get the full text version of all the transactions that have been taking place and save it on a file located on the main server.

Command for this operation:
~~~
./[clientname] TXLIST
~~~

### STATS
Provide a summary of all the transactions and a list of all the usernames who has made transactions with.

Command for this operation:
~~~
./[clientname] [username] stats
~~~
