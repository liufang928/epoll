GXX = g++
GXXFLAGS = -Wall -std=c++11 -pthread

Server = bin/server 
Client = bin/client

Test = bin/test

all: Server Client

Server: src/server.cpp
	$(GXX) $(GXXFLAGS) $< -o $(Server)

Client: src/client.cpp
	$(GXX) $(GXXFLAGS) $< -o $(Client)

Test: test/*.cpp
	$(GXX) $(GXXFLAGS) $< -o $@

clean:
	rm -rf bin/*