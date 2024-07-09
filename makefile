CXX ?= g++

DEBUG ?= 1
CXXFLAGS = -std=c++14
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

client:client.cpp
	$(CXX) -o client $^ $(CXXFLAGS) -lpthread

server: main.cpp ./http/*.cpp ./log/log.cpp ./mysql/sql_connection_pool.cpp webserver.cpp config.cpp epoller.cpp buffer.cpp inet_address.cpp acceptor.cpp event_loop.cpp socket.cpp connection.cpp channel.cpp threadpool.cpp tcpserver.cpp timestamp.cpp
	$(CXX) -I/usr/include/mysql -o server $^ $(CXXFLAGS) -lpthread -L/usr/lib/mysql -lmysqlclient

clean:
	rm  -r server
