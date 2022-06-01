#All
all: serverM serverA serverB serverC clientA clientB

#Servers
serverM: serverM.cpp
	g++ -std=c++11 -Wall -ggdb serverM.cpp -o serverM

serverA: serverA.cpp
	g++ -std=c++11 -Wall -ggdb serverA.cpp -o serverA

serverB: serverB.cpp
	g++ -std=c++11 -Wall -ggdb serverB.cpp -o serverB

serverC: serverC.cpp
	g++ -std=c++11 -Wall -ggdb serverC.cpp -o serverC

#Clients
clientA: clientA.cpp
	g++ -std=c++11 -Wall -ggdb clientA.cpp -o clientA

clientB: clientB.cpp
	g++ -std=c++11 -Wall -ggdb clientB.cpp -o clientB