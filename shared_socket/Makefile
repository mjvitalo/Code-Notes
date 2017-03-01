all: client server time-server

clean:
	  rm client server time-server

server: server.cpp
	  g++ -std=c++14 -o server server.cpp -lboost_system -lpthread

client: client.cpp
		g++ -std=c++14 -o client client.cpp -lboost_system -lpthread

time-server: time-server.cpp
		g++ -std=c++14 -o time-server time-server.cpp -lboost_system -lpthread
