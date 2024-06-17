GXX = g++
GXXFLAGS = -Wall -std=c++11 -pthread

Server = bin/server 
Client = bin/client

all: Server Client

Server: src/server2.cpp
	$(GXX) $(GXXFLAGS) $< -o $(Server)

Client: src/client.cpp
	$(GXX) $(GXXFLAGS) $< -o $(Client)

# 测试模块

Test = bin/test

Test: test/test_threadPoll.cpp
	$(GXX) $(GXXFLAGS) $< -o $(Test)

test: Test


clean:
	rm -rf bin/*