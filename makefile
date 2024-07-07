CXX ?= g++

DEBUG ?= 1
CXXFLAGS = -std=c++14 -Wall
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

server: main.cpp  ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./mysql/sql_connection_pool.cpp  webserver.cpp config.cpp epoller.cpp
	$(CXX) -I/usr/include/mysql -o server  $^ $(CXXFLAGS) -lpthread -L/usr/lib/mysql -lmysqlclient
#server: main.cpp  ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp  webserver.cpp config.cpp
#	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread

clean:
	rm  -r server
