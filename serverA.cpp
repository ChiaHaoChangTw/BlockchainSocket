#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<iostream>
#include<string>
#include<sstream>
#include<fstream>
#include<unordered_map>
#include<vector>

using namespace std;

#define PORT "21268"
#define IP_M "127.0.0.1"
#define PORT_UDPM "24268"
#define MAXBUFLEN 1000
#define FILENAME "block1.txt"

//==============================
//==============================
// Function Prototypes
//==============================
//==============================

//handle IP4 or IP6 addresses
void *get_in_addr(struct sockaddr *sa);

//create UDP socket and return sockfd
int createUDPSocket(struct addrinfo*& servinfo, struct addrinfo*& p);

//handle client UDP requests
void handleUDPClient(int sockfd, unordered_map<string, vector<string>>& logs, int& maxSerial);

//send UDP datagrams
void sendUDPData(int sockfd, string& message);

//handle check request
void handleCheckOp(int sockfd, string& username, unordered_map<string, vector<string>>& logs, int& maxSerial);

//initialize and store all data into logs hash map
void initLogs(unordered_map<string, vector<string>>& logs, int& maxSerial);

//update block file, logs, and maxSerial
void handleTxOp(int sockfd, int serial, string& username1, string& username2, int amount, unordered_map<string, vector<string>>& logs, int& maxSerial);

//handle TXLIST request
void handleListOp(int sockfd);

//handle stats request
void handleStatsOp(int sockfd, string& username, unordered_map<string, vector<string>>& logs);

//==============================
//==============================
// Main Processes
//==============================
//==============================

//Source: Beej's Guide to Network Programming - 6.3
int main(){

	//decalre vaiables ==========
	int sockfd, status, maxSerial = 0;
    struct addrinfo hints, *servinfo, *p;
    unordered_map<string, vector<string>> logs;
	//=============================

	//prepare hints ==========
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	//=============================

	//get address info ==========
	if((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
	//=============================

	//create UDP sockets ==========
	sockfd = createUDPSocket(servinfo, p);
	//=============================

	//memory management and check if bind successfully ==========
	freeaddrinfo(servinfo);
	if(p == NULL){
		fprintf(stderr, "serverA: failed to bind\n");
		exit(1);
	}
	//=============================

	//printf("serverA: waiting to recvfrom...\n");
	printf("The ServerA is up and running using UDP on port 21268.\n");

	//loop to handle UDP requests ==========
	while(true){
		handleUDPClient(sockfd, logs, maxSerial);
	}
	//=============================

	//close UDP socket ==========
 	close(sockfd);
 	//=============================

	return 0;
}

//==============================
//==============================
// Function Definitions
//==============================
//==============================

//handle IP4 or IP6 addresses
void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//create UDP socket and return sockfd
int createUDPSocket(struct addrinfo*& servinfo, struct addrinfo*& p){
	int sockfd;
	for(p = servinfo; p != NULL; p = p->ai_next){
		//create socket - socket()
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("serverA: socket");
			continue;
		}
		//bind to port - bind()
		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("serverA: bind");
			continue;
		}
		break;
	}
	return sockfd;
}

//handle client UDP requests
void handleUDPClient(int sockfd, unordered_map<string, vector<string>>& logs, int& maxSerial){
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
    cout << "The ServerA received a request from the Main Server." << endl;
    buf[numbytes] = '\0';
    //printf("ServerA: packet contains \"%s\"\n", buf);
    //initialize and store all data into logs hash map
    if(logs.size() == 0){
		initLogs(logs, maxSerial);
	}
    //process data
    istringstream iss(buf);
    string operation, username;
    iss >> operation >> username;
    if(operation == "CHECK"){
    	handleCheckOp(sockfd, username, logs, maxSerial);
    }
    if(operation == "TXCOINS"){
    	string username1, username2;
    	int amount, serial = stoi(username);
    	iss >> username1 >> username2 >> amount;
    	handleTxOp(sockfd, serial, username1, username2, amount, logs, maxSerial);
    }
    if(operation == "TXLIST"){
    	handleListOp(sockfd);
    }
    if(operation == "stats"){
    	handleStatsOp(sockfd, username, logs);
    }
}

//send UDP datagrams
void sendUDPData(int sockfd, string& message){

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
	if((status = getaddrinfo(IP_M, PORT_UDPM, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(1);
	}
    //=============================

    //send message ==========
	for(p = servinfo; p != NULL; p = p->ai_next){
		int numbytes;
		if((numbytes = sendto(sockfd, message.c_str(), strlen(message.c_str()), 0,
	             			  p->ai_addr, p->ai_addrlen)) == -1){
	        perror("serverA: sendto");
	    	continue;
	    }
	    printf("The ServerA finished sending the response to the Main Server.\n");
	    break;
	}
	//=============================

    //memory management and check if message sent successfully ==========
	freeaddrinfo(servinfo);
	if(p == NULL){
		fprintf(stderr, "serverA: failed to send\n");
		exit(1);
	}
	//=============================
}

//handle check request
void handleCheckOp(int sockfd, string& username, unordered_map<string, vector<string>>& logs, int& maxSerial){
	//send response to main server
    string message = "serverA ";
    if(logs.find(username) == logs.end()){
    	message += ("N " + to_string(maxSerial));
    	//serverA N
    }
    else{
    	int balance = 0;
    	for(auto& log: logs[username]){
    		istringstream iss(log);
			string username1, username2;
			int serial, amount;
    		iss >> serial >> username1 >> username2 >> amount;
    		if(username1 == username){balance -= amount;}
    		else{balance += amount;}
    	}
    	message += ("Y " + to_string(balance) + " " + to_string(maxSerial));
    	//serverA Y balance
    }
    sendUDPData(sockfd, message);
}

//initialize and store all data into logs hash map
void initLogs(unordered_map<string, vector<string>>& logs, int& maxSerial){
	ifstream infile;
	infile.open(FILENAME);
	string line;
	while(getline(infile, line)){
		if(line.size() > 0){
			istringstream iss(line);
			string username1, username2;
			int serial;
			iss >> serial >> username1 >> username2;
			logs[username1].push_back(line);
			logs[username2].push_back(line);
			maxSerial = max(maxSerial, serial);
		}	
	}
	infile.close();
}

//update block file, logs, and maxSerial
void handleTxOp(int sockfd, int serial, string& username1, string& username2, int amount, unordered_map<string, vector<string>>& logs, int& maxSerial){
	string newTransaction = to_string(serial) + " " + username1 + " " + username2 + " " + to_string(amount); 
	ofstream outfile;
	outfile.open(FILENAME, ios::app);
	outfile << newTransaction << endl;
	outfile.close();
	logs[username1].push_back(newTransaction);
	logs[username2].push_back(newTransaction);
	maxSerial = serial;
	string message = "TXCOINSOK";
	sendUDPData(sockfd, message);
}

//handle TXLIST request
void handleListOp(int sockfd){
	ifstream infile;
	infile.open(FILENAME);
	string line;
	vector<string> transVec;
	while(getline(infile, line)){
		if(line.size() > 0){
			transVec.push_back(line);
		}	
	}
	infile.close();
	string message = to_string(transVec.size());
	sendUDPData(sockfd, message);
	for(int i = 0 ; i < int(transVec.size()); ++i){
		sendUDPData(sockfd, transVec[i]);
	}
}

//handle stats request
void handleStatsOp(int sockfd, string& username, unordered_map<string, vector<string>>& logs){
	string message = to_string(logs[username].size());
	sendUDPData(sockfd, message);
	for(auto& log: logs[username]){
		sendUDPData(sockfd, log);
	}
}