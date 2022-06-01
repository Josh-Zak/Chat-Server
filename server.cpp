#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <list>

#include "RobustIO.h"

std::list<std::string> msgList; //Chat log
std::list<int> connList; //list of current connections
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* clientThread(void* data){
	int connection;
	connection = *(int*) data;

	//read most recent chats
	std::list<std::string>::iterator it;
	pthread_mutex_lock(&mutex);
	for(it = msgList.begin(); it != msgList.end(); it++){
		RobustIO::write_string(connection, *it);
	}
	pthread_mutex_unlock(&mutex);

	std::string name = RobustIO::read_string(connection);
	std::string msg;
	while(true){
		msg = RobustIO::read_string(connection);
		//if the user exits, exit loop to close the connection
		if(msg.compare("exit") == 0 || msg.compare("") == 0){
			break;
		}

		//send message to all connected clients
		std::list<int>::iterator it2;
		pthread_mutex_lock(&mutex);
		for(it2 = connList.begin(); it2 != connList.end(); it2++){
			if(*it2 != connection){
				RobustIO::write_string(*it2, name + ": " + msg);
			}
		}
		pthread_mutex_unlock(&mutex);

		// add the new msg to the list (thread safe)
		pthread_mutex_lock(&mutex);
		msgList.push_back(name + ": " + msg);
		if(msgList.size() > 12){
			msgList.pop_front();
		}
		pthread_mutex_unlock(&mutex);
	}
	//remove the connection from the list of active connections
	connList.remove(connection);

	//terminate the thread and close the connection
	close(connection);
	pthread_exit(0);
}

int main(int argc, char **argv) {
	int sock, conn;
	int i;
	int rc;
	struct sockaddr address;
	socklen_t addrLength = sizeof(address);
	struct addrinfo hints;
	struct addrinfo *addr;
	char buffer[512];
	int len;

	// Clear the address hints structure
    memset(&hints, 0, sizeof(hints));

    hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; // IPv4/6, socket is for binding
	// Get address info for local host
	if((rc = getaddrinfo(NULL, "55555", &hints, &addr))) {
		printf("host name lookup failed: %s\n", gai_strerror(rc));
		exit(1);
	}

	// Create a socket
    sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if(sock < 0) {
		printf("Can't create socket\n");
		exit(1);
	}

	// Set the socket for address reuse, so it doesn't complain about
	// other servers on the machine.
    i = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

	// Bind the socket
    rc = bind(sock, addr->ai_addr, addr->ai_addrlen);
    if(rc < 0) {
		printf("Can't bind socket\n");
		exit(1);
	}

	// Clear up the address data
    freeaddrinfo(addr);

	// Listen for new connections, wait on up to five of them
    rc = listen(sock, 5);
    if(rc < 0) {
		printf("listen failed\n");
		exit(1);
	}

	printf("====== SERVER IS RUNNING ======\n");

	// When we get a new connection, try reading some data from it!
	while((conn = accept(sock, (struct sockaddr*) &address, &addrLength)) >= 0){
		//create a new thread for the connection
		pthread_t t_id;
		int ret = pthread_create(&t_id, NULL, clientThread, &conn);
		if(ret != 0){
			printf("Cannot create thread.");
			exit(1);
		}
		//add current connection to the list
		connList.push_front(conn);
	}

}