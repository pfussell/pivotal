CXX=clang++
WARN=-Wall
OPTIMIZATION=-O0
CXXFLAGS=$(WARN) $(OPTIMIZATION)
EXE=pivotal

all:
	$(CXX) -std=c++11 $(CXXFLAGS) ProxyServer.cpp main.cpp -o $(EXE)