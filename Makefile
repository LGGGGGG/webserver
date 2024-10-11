CPP_FLAGS='-std=c++20'
simple_client:simple_client.cc
	g++ ${CPP_FLAGS} simple_client.cc -o simple_client