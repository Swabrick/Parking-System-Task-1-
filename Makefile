CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
SRC = src/main.cpp src/database.cpp src/parking.cpp src/utils.cpp src/http_server.cpp

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o smart_parking

windows:
	$(CXX) $(CXXFLAGS) $(SRC) -o smart_parking.exe -lws2_32

clean:
	rm -f smart_parking smart_parking.exe

run: all
	./smart_parking
