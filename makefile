DBGFLAGS=-g -fsanitize=address -fsanitize=undefined

#CCFLAGS=$(CFLAGS) -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -std=c++14 -pipe $(DBGFLAGS)

#SCFLAGS+=$(CFLAGS) -std=c++14 -pipe -pthread $(DBGFLAGS)

CFLAGS+=-std=c++14 -pipe -pthread -Wall -Wextra $(DBGFLAGS)

#-fsanitize=thread
#-fsanitize=address

CLIENTSRC=./src/client.cpp ./src/wofclient.cpp ./src/netqueue.cpp ./src/common.cpp
SERVERSRC=./src/server.cpp ./src/wofserver.cpp ./src/netqueue.cpp ./src/common.cpp
TESTSRC= ./src/test.cpp ./src/wofclient.cpp ./src/wofserver.cpp ./src/netqueue.cpp ./src/common.cpp

all: build

build: client server

client:
	g++ -o client.out $(CLIENTSRC) $(CFLAGS)

server:
	g++ -o server.out $(SERVERSRC) $(CFLAGS)

testbuild:
	rm test.out ; g++ -o test.out $(TESTSRC) $(CFLAGS) -lgtest
	chmod +x ./test.out

test: testbuild
	./test.out

clean:
	rm client.out
	rm server.out