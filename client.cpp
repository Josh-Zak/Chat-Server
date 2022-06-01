#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>

#include "RobustIO.h"

void* serverResponse(void* data){
    int socket;
	socket = *(int*) data;
    std::string response = "";
    while(true){
        response = RobustIO::read_string(socket);
        if(response != ""){
            printf("%s\n", response.c_str());
        }
        response = "";
    }
}


/* OPEN PROBLEMS
    NAMES DONT SWITCH BETWEEN USERS PROPERLY
    EVENTUALLY CLIENTS WILL GET BLOCKED (CANT WRITE TO SERVER)
*/


int main(int argc, char **argv) {
	struct addrinfo hints;
	struct addrinfo *addr;
	struct sockaddr_in *addrinfo;
	int rc;
	int sock;
	char buffer[512];
	int len;

    // Clear the data structure to hold address parameters
    memset(&hints, 0, sizeof(hints));

    // TCP socket, IPv4/IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    // Get address info for local host
    rc = getaddrinfo("localhost", NULL, &hints, &addr);
    if (rc != 0) {
        // Note how we grab errors here
        printf("Hostname lookup failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    // Copy the address info, use it to create a socket
    addrinfo = (struct sockaddr_in *) addr->ai_addr;

    sock = socket(addrinfo->sin_family, addr->ai_socktype, addr->ai_protocol);
    if (sock < 0) {
        printf("Can't connect to server\n");
		exit(1);
    }

    // Make sure port is in network order
    addrinfo->sin_port = htons(55555);

    // Connect to the server using the socket and address we resolved
    rc = connect(sock, (struct sockaddr *) addrinfo, addr->ai_addrlen);
    if (rc != 0) {
        printf("Connection failed\n");
        exit(1);
    }

    // Clear the address struct
    freeaddrinfo(addr);

    if(argc > 2){
        printf("Too many args\n");
        exit(1);
    }else if(argc < 2){
        printf("Not enough args\n");
        exit(1);
    }


    //create a thread to handle server responses
    pthread_t t_id;
    int ret = pthread_create(&t_id, NULL, serverResponse, &sock);
    if(ret != 0){
        printf("Cannot create thread.");
    }

    //send the name to the server
    std::string name = argv[1];
    RobustIO::write_string(sock, name);

    std::string message;
    getline(std::cin, message);
    while(message.compare("exit") != 0){
        //send messages here
        RobustIO::write_string(sock, message);
        getline(std::cin, message);
    }

    //tell the server we're exiting
    RobustIO::write_string(sock, message);

    //kill the thread and close the socket
    pthread_cancel(t_id);
    if(ret != 0){
        printf("Cannot kill thread.");
    }
	close(sock);
}