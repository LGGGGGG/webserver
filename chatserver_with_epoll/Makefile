CPP_FLAGS='-std=c++20'
all:server client
server:server.cc

	g++ ${CPP_FLAGS} $^ -o $@
client:client.cc
	g++ ${CPP_FLAGS} $^ -o $@
clean:

	rm server client
