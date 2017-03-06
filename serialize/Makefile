all: simple_one simple_two

clean:
	  rm simple_one  simple_two 

simple_one: simple_one.cpp
	  g++ -std=c++14 -o simple_one simple_one.cpp -lboost_system -lboost_serialization -lpthread

simple_two: simple_two.cpp
	  g++ -std=c++14 -o simple_two simple_two.cpp -ltr -lboost_system -lboost_serialization -lpthread

