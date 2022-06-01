# Chat-Server

# Description
Simple command line chat server that allows for multiple clients to connect and chat all at the same time by using pthreads.
# How to Run
1. Compile the server with `g++ server.cpp RobustIO.cpp -pthread`
2. Run the server with no args
3. Compile each client with `g++ client.cpp RobustIO.cpp -pthread`
4. Run each client with the clients name as the only arg

# Features
- Stores a log of the most recent messages to send to new clients
- Name of the user is sent along with the message
