#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<signal.h>
#include<sys/select.h>
#include<algorithm>
#include<string>
#include<sstream>
#include<vector>
#include<utility>
#include<fstream>
#include<unordered_map>
#include<tuple>

using namespace std;

#define PORT_A "25268" //serverM
#define PORT_B "26268" //serverM
#define PORT_UDP "24268" //serverM
#define PORT_UDPA "21268" //serverA
#define PORT_UDPB "22268" //serverB
#define PORT_UDPC "23268" //serverC
#define IP_A "127.0.0.1" 
#define IP_B "127.0.0.1" 
#define IP_C "127.0.0.1" 
#define BACKLOG 20
#define MAXBUFLEN 1500
#define MAXDATASIZE 1500
#define FILENAME "alichain.txt"

//==============================
//==============================
// Function Prototypes
//==============================
//==============================

//handle child processes to avoid zombie processes
void sigchld_handler(int s);

//handle IP4 or IP6 addresses
void *get_in_addr(struct sockaddr *sa);

//create TCP parent socket and return sockfd
int createParentSocket(struct addrinfo*& servinfo, struct addrinfo*& p);

//handle client A or B
void handleClient(int sockfd, int sockfd_udp);

//create UDP socket and return sockfd
int createUDPSocket(struct addrinfo*& servinfo, struct addrinfo*& p);

//handle client UDP requests
void handleUDPClient(int sockfd);

//handle check request
void handleCheckOp(int newfd, int sockfd_udp, string& client, string& username);

//recv check responses from backend servers and return max serial number
int recvCheckResponse(int sockfd_udp, bool& isFound, int& balance);

//handle TXCOINS request
void handleTxOp(int newfd, int sockfd_udp, string& client, string& username1, string& username2, int amount);

//send UDP datagrams
void sendUDPData(int sockfd, string& message, char server);

//recv backend server's UDP response
string recvUDPData(int sockfd);

//check information of sender and reciever, and return true if the transaction can be done
bool checkTwoUsrs(int sockfd_udp, string& username1, string& username2, int amount, string& message, int& balanceUsr1, int& balanceUsr2);

//execute transfer coins to one of the backend server
void exeTransfer(int sockfd_udp, string& message, string& username1, string& username2, int amount, int balanceUsr1);

//handle TXLIST request
void handleListOp(int newfd, int sockfd_udp, string& client);

//Get all the transactions from a backend server and push back to the vector
void getTransactions(int sockfd_udp, char server, vector<pair<int, string>>& transVec);

//handle stats request
void handleStatsOp(int newfd, int sockfd_udp, string& client, string& username);

//Get transactions of username from a backend server and store in hash map
void getUsrTransactions(int sockfd_udp, char server, unordered_map<string, vector<int>>& transMap, string& username);	

//==============================
//==============================
// Main Processes
//==============================
//==============================

//Source: Beej's Guide to Network Programming - 6.1
int main(){

	//decalre vaiables ==========
	int sockfd_A, sockfd_B, sockfd_udp, status;
	struct addrinfo hints, hints_udp, *servinfo_A, *servinfo_B, *servinfo_udp, *p_A, *p_B, *p_udp;
	struct sigaction sa; 
	fd_set readfds;
	//=============================

	//prepare hints for TCP ==========
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	//=============================

	//prepare hints for UDP ==========
	memset(&hints_udp, 0, sizeof hints_udp);
	hints_udp.ai_family = AF_UNSPEC;
	hints_udp.ai_socktype = SOCK_DGRAM;
	hints_udp.ai_flags = AI_PASSIVE;
	//=============================

	//get address info for port A, B, and UDP ==========
	if((status = getaddrinfo(NULL, PORT_A, &hints, &servinfo_A)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
	if((status = getaddrinfo(NULL, PORT_B, &hints, &servinfo_B)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
	if((status = getaddrinfo(NULL, PORT_UDP, &hints_udp, &servinfo_udp)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
	//=============================

	//create TCP parent sockets for A and B ==========
	sockfd_A = createParentSocket(servinfo_A, p_A);
	sockfd_B = createParentSocket(servinfo_B, p_B);
	//=============================

	//create UDP sockets ==========
	sockfd_udp = createUDPSocket(servinfo_udp, p_udp);
	//=============================

	//memory management and check if bind successfully ==========
	freeaddrinfo(servinfo_A);
	freeaddrinfo(servinfo_B);
	freeaddrinfo(servinfo_udp);
	if(p_A == NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	if(p_B == NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	if(p_udp == NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	//=============================

	//listen() on A and B ==========
	if(listen(sockfd_A, BACKLOG) == -1){
		perror("listen");
		exit(1);
	}
	if(listen(sockfd_B, BACKLOG) == -1){
		perror("listen");
		exit(1);
	}
	//=============================

	//reap all dead processes ==========
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    //=============================

    //printf("server: waiting for connections...\n");
    //printf("serverA: waiting to recvfrom...\n");
    printf("The main server is up and running.\n");

    //accept loop for A and B ==========
    while(true){
    	FD_ZERO(&readfds);
    	FD_SET(sockfd_A, &readfds), FD_SET(sockfd_B, &readfds);
    	status = select(max(sockfd_A, sockfd_B) + 1, &readfds, NULL, NULL, NULL);
    	if(status == -1){
			//perror("select");
    	}
    	else{
    		if(FD_ISSET(sockfd_A, &readfds)){
    			handleClient(sockfd_A, sockfd_udp);
    		}
    		if(FD_ISSET(sockfd_B, &readfds)){
    			handleClient(sockfd_B, sockfd_udp);
    		}
    	}
    }
    //=============================
 	
 	//close TCP parent and UDP sockets ==========
 	close(sockfd_A), close(sockfd_B), close(sockfd_udp);
 	//=============================

	return 0;
}

//==============================
//==============================
// Function Definitions
//==============================
//==============================

//handle child processes to avoid zombie processes
void sigchld_handler(int s){
	int saved_errno = errno;
	//https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-waitpid-wait-specific-child-process-end
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}

//handle IP4 or IP6 addresses
void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//create TCP parent socket and return sockfd
int createParentSocket(struct addrinfo*& servinfo, struct addrinfo*& p){
	int sockfd;
	int yes = 1;
	for(p = servinfo; p != NULL; p = p->ai_next){
		//create socket - socket()
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("server: socket");
			continue;
		}
		//allow other sockets to bind() to this port
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			perror("setsockopt");
			exit(1);
		}
		//bind to port - bind()
		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	return sockfd;
}

//handle client A or B
void handleClient(int sockfd, int sockfd_udp){
	int newfd;
	socklen_t sin_size;
	struct sockaddr_storage their_addr;
	char s[INET6_ADDRSTRLEN];
	//accept()
	sin_size = sizeof their_addr;
	newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if(newfd == -1){
		perror("accept");
		return;
	}
	//print client address
	inet_ntop(their_addr.ss_family,
        	  get_in_addr((struct sockaddr *)&their_addr),
        	  s, sizeof s);
    //printf("server: got connection from %s\n", s);
    //child process
    if(!fork()){ //https://www.geeksforgeeks.org/fork-system-call/
    	close(sockfd);
    	//recv()
    	int numbytes;  
	    char buf[MAXDATASIZE];
		if((numbytes = recv(newfd, buf, MAXDATASIZE - 1, 0)) == -1){
	        perror("recv");
	        exit(1);
	    }
	    buf[numbytes] = '\0';
	    istringstream iss(buf);
	    string client, operation, username;
	    iss >> client >> operation >> username;
	    if(operation == "CHECK"){handleCheckOp(newfd, sockfd_udp, client, username);}
	    if(operation == "TXCOINS"){
	    	string username2;
	    	int amount;
	    	iss >> username2 >> amount;
			handleTxOp(newfd, sockfd_udp, client, username, username2, amount);
	    }
	    if(operation == "TXLIST"){handleListOp(newfd, sockfd_udp, client);}
	    if(operation == "stats"){handleStatsOp(newfd, sockfd_udp, client, username);}
	    //close()
    	close(newfd);
    	exit(0);
    }
    close(newfd);
}

//create UDP socket and return sockfd
int createUDPSocket(struct addrinfo*& servinfo, struct addrinfo*& p){
	int sockfd;
	for(p = servinfo; p != NULL; p = p->ai_next){
		//create socket - socket()
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("server: socket");
			continue;
		}
		//bind to port - bind()
		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	return sockfd;
}

//handle client UDP requests
void handleUDPClient(int sockfd){
	int numbytes;
	socklen_t addr_len;
	struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];
    //recvfrom()
    addr_len = sizeof their_addr;
    if((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, 
    	(struct sockaddr *)&their_addr, &addr_len)) == -1){
    	perror("recvfrom");
    	exit(1);
    }
    printf("Server: got packet from %s\n",
           inet_ntop(their_addr.ss_family,
		              get_in_addr((struct sockaddr *)&their_addr),
		              s, sizeof s));
    printf("Server: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("Server: packet contains \"%s\"\n", buf);
}

//handle check request
void handleCheckOp(int newfd, int sockfd_udp, string& client, string& username){
	//print request received from client message
	cout << "The main server received input=" + username + 
	" from the client using TCP over port ";
	if(client == "A"){printf("" PORT_A ".\n");}
	else{printf("" PORT_B ".\n");}
	//send requests to backend servers
	string message = "CHECK " + username;
	sendUDPData(sockfd_udp, message, 'A'), sendUDPData(sockfd_udp, message, 'B'), sendUDPData(sockfd_udp, message, 'C');
	//recv responses from backend servers
	bool isFound = false;
	int balance = 1000;
	recvCheckResponse(sockfd_udp, isFound, balance), recvCheckResponse(sockfd_udp, isFound, balance), recvCheckResponse(sockfd_udp, isFound, balance);
	string responseToClient;
	if(isFound){responseToClient = "The current balance of " + username + " is : " + to_string(balance) + " alicoins.\n";}
	else{responseToClient = "Unable to proceed with the request as " + username + " is not part of the network.\n";} 
	//send() to TCP client
	//if(send(newfd, "Hello, world! I am CHECK.\n", 30, 0) == -1){
	if(send(newfd, responseToClient.c_str(), responseToClient.size() + 1, 0) == -1){
		perror("send");
	}
	cout << "The main server sent the current balance to client " << client << " ." << endl; 
}

//recv check responses from backend servers and return max serial number
int recvCheckResponse(int sockfd_udp, bool& isFound, int& balance){
	string server, found, port;
	int amount, maxSerial = 0;
	istringstream iss(recvUDPData(sockfd_udp));
	iss >> server >> found;
	server = ((server == "serverA") ? "A" : ((server == "serverB") ? "B" : "C"));
	port = ((server == "A") ? PORT_UDPA : ((server == "B") ? PORT_UDPB : PORT_UDPC));
	if(found == "Y"){
		isFound = true;
		iss >> amount >> maxSerial;
		balance += amount;
	}
	else{
		iss >> maxSerial;
	}
	printf("The main server received transactions from Server %s using UDP over port %s.\n", server.c_str(), port.c_str());
	return maxSerial;
}

//handle TXCOINS request
void handleTxOp(int newfd, int sockfd_udp, string& client, string& username1, string& username2, int amount){
	//print request received from client message
	printf("The main server received from %s to transfer %d coins to %s", username1.c_str(), amount, username2.c_str());
	if(client == "A"){printf(" using TCP over port " PORT_A ".\n");}
	else{printf(" using TCP over port " PORT_B ".\n");}
	//Check information of sender and receiver
	bool canTransfer = false;
	string message = "";
	int balanceUsr1 = 1000, balanceUsr2 = 1000;
	canTransfer = checkTwoUsrs(sockfd_udp, username1, username2, amount, message, balanceUsr1, balanceUsr2);
	if(canTransfer){
		exeTransfer(sockfd_udp, message, username1, username2, amount, balanceUsr1);
	}
	//send() to TCP client
	if(send(newfd, message.c_str(), message.size() + 1, 0) == -1){
		perror("send");
	}
	cout << "The main server sent the result of the transaction to client " << client << " ." << endl; 
}

//send UDP datagrams
void sendUDPData(int sockfd, string& message, char server){

	//decalre vaiables ==========
	int status;
    struct addrinfo hints, *servinfo, *p;
	//=============================

    //prepare hints ==========
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    //=============================

    //get address info ==========
    if(server == 'A'){
    	if((status = getaddrinfo(IP_A, PORT_UDPA, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			exit(1);
		}
    }
    else if(server == 'B'){
    	if((status = getaddrinfo(IP_B, PORT_UDPB, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			exit(1);
		}
    }
    else{
    	if((status = getaddrinfo(IP_C, PORT_UDPC, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			exit(1);
		}
    }
    //=============================

    //send message ==========
	for(p = servinfo; p != NULL; p = p->ai_next){
		int numbytes;
		if((numbytes = sendto(sockfd, message.c_str(), strlen(message.c_str()), 0,
	             			  p->ai_addr, p->ai_addrlen)) == -1){
	        perror("serverM: sendto");
	    	continue;
	    }
	    printf("The main server sent a request to server %c.\n", server);
	    break;
	}
	//=============================

    //memory management and check if message sent successfully ==========
	freeaddrinfo(servinfo);
	if(p == NULL){
		fprintf(stderr, "serverM: failed to send\n");
		exit(1);
	}
	//=============================
}

//recv backend server's UDP response
string recvUDPData(int sockfd){
	int numbytes;
	socklen_t addr_len;
	struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    //recvfrom()
    addr_len = sizeof their_addr;
    if((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, 
    	(struct sockaddr *)&their_addr, &addr_len)) == -1){
    	perror("recvfrom");
    	exit(1);
    }
    buf[numbytes] = '\0';
    //printf("Server: packet contains \"%s\"\n", buf);
    return string(buf);
}

//check information of sender and reciever, and return true if the transaction can be done
bool checkTwoUsrs(int sockfd_udp, string& username1, string& username2, int amount, string& message, int& balanceUsr1, int& balanceUsr2){
	//send username1 CHECK requests to backend servers
	string messageUsr1 = "CHECK " + username1;
	sendUDPData(sockfd_udp, messageUsr1, 'A'), sendUDPData(sockfd_udp, messageUsr1, 'B'), sendUDPData(sockfd_udp, messageUsr1, 'C');
	//recv username1 CHECK responses from backend servers, and get max serial number
	bool isFoundUsr1 = false;
	int maxSerial;
	maxSerial = max({recvCheckResponse(sockfd_udp, isFoundUsr1, balanceUsr1), recvCheckResponse(sockfd_udp, isFoundUsr1, balanceUsr1), recvCheckResponse(sockfd_udp, isFoundUsr1, balanceUsr1)});
	//send username2 CHECK requests to backend servers
	string messageUsr2 = "CHECK " + username2;
	sendUDPData(sockfd_udp, messageUsr2, 'A'), sendUDPData(sockfd_udp, messageUsr2, 'B'), sendUDPData(sockfd_udp, messageUsr2, 'C');
	//recv username2 CHECK responses from backend servers
	bool isFoundUsr2 = false;
	recvCheckResponse(sockfd_udp, isFoundUsr2, balanceUsr2), recvCheckResponse(sockfd_udp, isFoundUsr2, balanceUsr2), recvCheckResponse(sockfd_udp, isFoundUsr2, balanceUsr2);
	//determine if the transaction can be done and prepare message to send
	if(!isFoundUsr1 && !isFoundUsr2){
		message = "Unable to proceed with the transaction as " + username1 + " and " + username2 + " are not part of the network.\n";
		return false;
	}
	if(!isFoundUsr1){
		message = "Unable to proceed with the transaction as " + username1 + " is not part of the network.\n";
		return false;
	}
	if(!isFoundUsr2){
		message = "Unable to proceed with the transaction as " + username2 + " is not part of the network.\n";
		return false;
	}
	if(balanceUsr1 < amount){
		message = username1 + " was unable to transfer " + to_string(amount) + " alicoins to " + username2 + " because of insufficient balance.\nThe current balance of " + username1 + " is " + to_string(balanceUsr1) + " alicoins.\n";
		return false;
	}
	message = "TXCOINS " + to_string(maxSerial + 1) + " " + username1 + " " + username2 + " " + to_string(amount);
	return true;
}

//execute transfer coins to one of the backend server
void exeTransfer(int sockfd_udp, string& message, string& username1, string& username2, int amount, int balanceUsr1){
	vector<char> servers = {'A', 'B', 'C'};
	srand(time(0));
	char server = servers[rand() % 3];
	sendUDPData(sockfd_udp, message, server);
	if(recvUDPData(sockfd_udp) == "TXCOINSOK"){
		string port = ((server == 'A') ? PORT_UDPA : ((server == 'B') ? PORT_UDPB : PORT_UDPC));
		printf("The main server received the feedback from server %c using UDP over port %s.\n", server, port.c_str());
	}
	else{
		cout << "TXCOINS Failed" << endl;
	}
	message = username1 + " successfully transferred " + to_string(amount) + " alicoins to " + username2 + " .\nThe current balance of " + username1 + " is: " + to_string(balanceUsr1 - amount) + " alicoins.\n";
}


//handle TXLIST request
void handleListOp(int newfd, int sockfd_udp, string& client){
	//print request received from client message
	printf("The main server received sorted list request from clent %s", client.c_str());
	if(client == "A"){printf(" using TCP over port " PORT_A ".\n");}
	else{printf(" using TCP over port " PORT_B ".\n");}
	//Get transactions from backend servers
	vector<pair<int, string>> transVec;
	getTransactions(sockfd_udp, 'A', transVec), getTransactions(sockfd_udp, 'B', transVec), getTransactions(sockfd_udp, 'C', transVec);	
	sort(transVec.begin(), transVec.end());
	ofstream outfile;
	outfile.open(FILENAME);
	for(int i = 0 ; i < int(transVec.size()); ++i){
		outfile << transVec[i].second << endl;
	}
	outfile.close();
	//send() to TCP client
	string message = "The sorted list is ready.\n";
	if(send(newfd, message.c_str(), message.size() + 1, 0) == -1){
		perror("send");
	}
	cout << "The main server has finished preparing sorted list file." << endl; 	
}

//Get all the transactions from a backend server and push back to the vector
void getTransactions(int sockfd_udp, char server, vector<pair<int, string>>& transVec){
	//send udp to backends
	string message = "TXLIST";
	sendUDPData(sockfd_udp, message, server);
	//recv udp from backends
	string port = ((server == 'A') ? PORT_UDPA : ((server == 'B') ? PORT_UDPB : PORT_UDPC));
	int numTrans = stoi(recvUDPData(sockfd_udp));
	printf("The main server received transactions from server %c using UDP over port %s.\n", server, port.c_str());
	while(numTrans > 0){
		string response = recvUDPData(sockfd_udp);
		istringstream iss(response);
		int serial;
		iss >> serial;
		transVec.push_back(make_pair(serial, response));
		--numTrans;
		printf("The main server received transactions from server %c using UDP over port %s.\n", server, port.c_str());
	}
}	

//handle stats request
void handleStatsOp(int newfd, int sockfd_udp, string& client, string& username){
	//print request received from client message
	printf("The main server received stats request from clent %s", client.c_str());
	if(client == "A"){printf(" using TCP over port " PORT_A ".\n");}
	else{printf(" using TCP over port " PORT_B ".\n");}
	//Get transactions of username from backend servers
	unordered_map<string, vector<int>> transMap; //Key: username //Value: #trans, total amount
	getUsrTransactions(sockfd_udp, 'A', transMap, username), getUsrTransactions(sockfd_udp, 'B', transMap, username), getUsrTransactions(sockfd_udp, 'C', transMap, username);	
	//transform map into tuple vector: #trans, total amount, username
	vector<tuple<int, int, string>> transVec;
	for(auto& it: transMap){
		transVec.push_back(make_tuple(it.second[0], it.second[1], it.first));
	}
	sort(transVec.begin(), transVec.end()); //need to reverse to use
	//send to client
	string message = "";
	for(int i = int(transVec.size()) - 1; i >= 0; --i){ //from big to small
		message += to_string(int(transVec.size()) - i) + " " + get<2>(transVec[i]) + " " + to_string(get<0>(transVec[i])) + " " + to_string(get<1>(transVec[i])) + "\n";
	}
	if(send(newfd, message.c_str(), message.size() + 1, 0) == -1){
		perror("send");
	}
	cout << "The main server sent the results of stats to client " << client << "." << endl; 	
}

//Get transactions of username from a backend server and store in hash map
void getUsrTransactions(int sockfd_udp, char server, unordered_map<string, vector<int>>& transMap, string& username){
	//send udp to backends
	string message = "stats " + username;
	sendUDPData(sockfd_udp, message, server);
	//recv udp from backends
	string port = ((server == 'A') ? PORT_UDPA : ((server == 'B') ? PORT_UDPB : PORT_UDPC));
	int numTrans = stoi(recvUDPData(sockfd_udp));
	printf("The main server received transactions from server %c using UDP over port %s.\n", server, port.c_str());
	while(numTrans > 0){
		string response = recvUDPData(sockfd_udp);
		istringstream iss(response);
		int serial, amount;
		string username1, username2;
		iss >> serial >> username1 >> username2 >> amount;
		//user is sender/username1 //Key: username //Value: #trans, total amount
		if(username1 == username){
			//receiver info is not in the map
			if(transMap.find(username2) == transMap.end()){transMap[username2] = {1, -amount};}	
			//receiver info is in the map
			else{transMap[username2][0] += 1, transMap[username2][1] -= amount;}
		}
		//user is receiver/username2
		else{
			//sender info is not in the map
			if(transMap.find(username1) == transMap.end()){transMap[username1] = {1, amount};}
			//sender info is in the map
			else{transMap[username1][0] += 1, transMap[username1][1] += amount;}

		}
		--numTrans;
		printf("The main server received transactions from server %c using UDP over port %s.\n", server, port.c_str());
	}
}