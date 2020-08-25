all: client.cpp connectionTime.cpp error_handler.cpp 
	clang++ -std=c++14 -o client client.cpp connectionTime.cpp error_handler.cpp -lpthread
