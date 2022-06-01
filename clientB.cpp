#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<iostream>
#include<string>
#include<sstream>
#include<iomanip>

using namespace std;

#define IP_M "127.0.0.1"
#define PORT_M "26268" //the port client will be connecting to 
#define MAXDATASIZE 1500 //max number of bytes we can get at once 

//==============================
//==============================
// Function Prototypes
//==============================
//==============================

//handle IP4 or IP6 addresses
void *get_in_addr(struct sockaddr *sa);

//create TCP socket, connect, and return sockfd
int createTCPSocket(struct addrinfo*& servinfo, struct addrinfo*& p);

//handle TCP send() and recv()
void handleTCPData(int sockfd, int argc, char* argv[]);

//handle check request
void handleCheckOp(int sockfd, char* argv[]);

//handle TXCOINS request
void handleTxOp(int sockfd, char* argv[]);

//handle TXLIST request
void handleListOp(int sockfd, char* argv[]);

//handle stats request
void handleStatsOp(int sockfd, char* argv[]);

//recv TCP data and return in string format 
string recvTCPData(int sockfd);

//==============================
//==============================
// Main Processes
//==============================
//==============================

//Source: Beej's Guide to Network Programming - 6.2
int main(int argc, char *argv[]){

    //decalre vaiables ==========
    int sockfd, status;  
    char s[INET6_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *p;
    //=============================

    //check command format ==========
    if(argc < 2){
        fprintf(stderr, "usage: client username || client username1 username2 amount || client TXLIST || client username stats\n");
        exit(1);
    }
    //=============================

    //prepare hints ==========
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //=============================

    //get address info ==========
    if((status = getaddrinfo(IP_M, PORT_M, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    //=============================
    
    //create TCP socket and connect to server ==========
    sockfd = createTCPSocket(servinfo, p);
    //=============================

    //memory management and check if connect successfully ==========
    freeaddrinfo(servinfo);
    if(p == NULL){
        fprintf(stderr, "clientB: failed to connect\n");
        exit(1);
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    //printf("clientB: connecting to %s\n", s);
    printf("The client B is up and running.\n");
    //=============================

    //handle TCP send() and recv() ==========
    handleTCPData(sockfd, argc, argv);
    //=============================

    //close TCP socket ==========
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

//create TCP socket, connect, and return sockfd
int createTCPSocket(struct addrinfo*& servinfo, struct addrinfo*& p){
    int sockfd;
    for(p = servinfo; p != NULL; p = p->ai_next){
        //create socket - socket()
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("clientB: socket");
            continue;
        }
        //connect to server - connect()
        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("clientB: connect");
            continue;
        }
        break;
    }
    return sockfd;
}

//handle TCP send() and recv()
void handleTCPData(int sockfd, int argc, char* argv[]){
    //CHECK or TXLIST
    if(argc == 2){
        if(string(argv[1]) != "TXLIST"){handleCheckOp(sockfd, argv);}
        else{handleListOp(sockfd, argv);}
    }
    //STATS
    else if(argc == 3){
        if(string(argv[2]) == "stats"){handleStatsOp(sockfd, argv);}
        else{cout << "Invalid input format. Please use: client username stats" << endl;}
    }
    //argc == 4
    else{handleTxOp(sockfd, argv);}
}

//handle check request
void handleCheckOp(int sockfd, char* argv[]){
    //send()
    string messageSend = "B CHECK " + string(argv[1]) + "\n";
    if(send(sockfd, messageSend.c_str(), messageSend.size() + 1, 0) == -1){
        perror("send");
        exit(1);
    }
    printf("%s sent a balance enquiry request to the main server.\n", argv[1]);
    //recv()
    int numbytes;  
    char buf[MAXDATASIZE];
    if((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1){
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s",buf);
}

//handle TXCOINS request
void handleTxOp(int sockfd, char* argv[]){
    //send() - B TXCOINS username1 username2 amount
    string messageSend = "B TXCOINS " + string(argv[1]) + " " + 
                         string(argv[2]) + " " + string(argv[3]) + "\n";
    if(send(sockfd, messageSend.c_str(), messageSend.size() + 1, 0) == -1){
        perror("send");
        exit(1);
    }
    printf("%s has requested to transfer %s coins to %s.\n", argv[1], argv[3], argv[2]);
    //recv()
    int numbytes;  
    char buf[MAXDATASIZE];
    if((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1){
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s",buf);
}

//handle TXLIST request
void handleListOp(int sockfd, char* argv[]){
    //send() - B TXLIST
    string messageSend = "B TXLIST\n";
    if(send(sockfd, messageSend.c_str(), messageSend.size() + 1, 0) == -1){
        perror("send");
        exit(1);
    }
    printf("ClientB sent a sorted list request to the main server.\n");
    //recv()
    int numbytes;  
    char buf[MAXDATASIZE];
    if((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1){
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s",buf);
}

//handle stats request
void handleStatsOp(int sockfd, char* argv[]){
    //send() - B stats username
    string messageSend = "B stats " + string(argv[1]) + "\n";
    if(send(sockfd, messageSend.c_str(), messageSend.size() + 1, 0) == -1){
        perror("send");
        exit(1);
    }
    printf("%s sent a statistics enqury request to the main server.\n", argv[1]);
    //recv()
    istringstream iss(recvTCPData(sockfd));
    string line;
    printf("%s statistics are the following.:\n", argv[1]);
    cout << setw(10) << "Rank" << setw(15) << "Username" << setw(25) << "NumofTransactions" << setw(15) << "Total" << endl;
    while(getline(iss, line)){
        istringstream issLine(line);
        string rank, username, numTrans, amount;
        issLine >> rank >> username >> numTrans >> amount;
        cout << setw(10) << rank << setw(15) << username << setw(25) << numTrans << setw(15) << amount << endl;
    }
}

//recv TCP data and return in string format 
string recvTCPData(int sockfd){
    int numbytes;  
    char buf[MAXDATASIZE];
    if((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1){
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    return string(buf);
}