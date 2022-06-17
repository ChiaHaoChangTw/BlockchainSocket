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

Command for this operation:
~~~
./[clientname] TXLIST
~~~
