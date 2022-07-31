CXX=g++
CXXFLAGS=-std=c++11 -O2 -g

TARGET:=myserver
SOURCE:=$(wildcard ../*.cpp)
OBJS=./buffer.cpp ./threadpool.cpp ./HTTPrequest.cpp ./HTTPresponse.cpp ./HTTPconnection.cpp \
     ./timer.cpp ./epoller.cpp ./webserver.cpp ./main.cpp

$(TARGET):$(OBJS)
	$(CXX) $(CXXFLAGS)  $(OBJS) -o ./bin/$(TARGET) -pthread

